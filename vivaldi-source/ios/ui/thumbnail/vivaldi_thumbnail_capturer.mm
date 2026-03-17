// Copyright 2026 Vivaldi Technologies. All rights reserved.

#import "ios/ui/thumbnail/vivaldi_thumbnail_capturer.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

namespace {
constexpr NSInteger kPoolLimit = 5;
constexpr NSTimeInterval kSnapshotDelaySeconds = 5.0;
static NSString* const kEstimatedProgressKey = @"estimatedProgress";
static void* kEstimatedProgressContext = &kEstimatedProgressContext;
}  // namespace

@interface VivaldiThumbnailCapturerTask : NSObject
@property(nonatomic, strong) NSURL* url;
@property(nonatomic, copy) VivaldiThumbnailCapturerCompletion completion;
@end

@implementation VivaldiThumbnailCapturerTask
@end

@interface VivaldiThumbnailCapturer () <WKNavigationDelegate>
@property(nonatomic, strong)
    NSMapTable<WKWebView*, VivaldiThumbnailCapturerTask*>* webViewTaskMap;
@property(nonatomic, strong) NSMutableArray<WKWebView*>* webViewPool;
@property(nonatomic, strong)
    NSMutableArray<VivaldiThumbnailCapturerTask*>* taskQueue;
@property(nonatomic, copy)
    WKWebViewConfiguration* _Nullable (^configurationProvider)(void);
@end

@implementation VivaldiThumbnailCapturer

- (instancetype)init {
  return [self initWithConfigurationProvider:nil];
}

- (instancetype)initWithConfigurationProvider:
    (WKWebViewConfiguration* _Nullable (^)(void))configurationProvider {
  self = [super init];
  if (self) {
    _configurationProvider = [configurationProvider copy];
    _webViewTaskMap =
        [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsStrongMemory |
                                           NSPointerFunctionsObjectPersonality
                              valueOptions:NSPointerFunctionsStrongMemory];
    _webViewPool = [[NSMutableArray alloc] init];
    _taskQueue = [[NSMutableArray alloc] init];
    [self setUpWebViews];
  }
  return self;
}

- (void)dealloc {
  for (WKWebView* webView in self.webViewPool) {
    [webView removeObserver:self
                 forKeyPath:kEstimatedProgressKey
                    context:kEstimatedProgressContext];
  }
  for (WKWebView* webView in self.webViewTaskMap.keyEnumerator) {
    [webView removeObserver:self
                 forKeyPath:kEstimatedProgressKey
                    context:kEstimatedProgressContext];
  }
}

#pragma mark - Public

- (void)captureSnapshotWithURL:(NSURL*)url
                    completion:(VivaldiThumbnailCapturerCompletion)completion {
  if (!url || !url.absoluteString.length) {
    return;
  }

  NSURL* targetURL = [NSURL URLWithString:url.absoluteString];
  if (!targetURL) {
    return;
  }

  VivaldiThumbnailCapturerTask* task =
      [[VivaldiThumbnailCapturerTask alloc] init];
  task.url = targetURL;
  task.completion = completion;

  WKWebView* webView = self.webViewPool.lastObject;
  if (webView) {
    [self.webViewPool removeLastObject];
    [self.webViewTaskMap setObject:task forKey:webView];
    [webView loadRequest:[NSURLRequest requestWithURL:targetURL]];
  } else {
    [self.taskQueue addObject:task];
  }
}

#pragma mark - Private

- (void)setUpWebViews {
  for (NSInteger index = 0; index < kPoolLimit; index++) {
    WKWebViewConfiguration* configuration =
        self.configurationProvider ? self.configurationProvider()
                                   : [[WKWebViewConfiguration alloc] init];
    WKWebView* webView =
        [[WKWebView alloc] initWithFrame:UIScreen.mainScreen.bounds
                           configuration:configuration];
    webView.navigationDelegate = self;

    [webView addObserver:self
              forKeyPath:kEstimatedProgressKey
                 options:NSKeyValueObservingOptionNew
                 context:kEstimatedProgressContext];

    [self.webViewPool addObject:webView];
  }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  if (context != kEstimatedProgressContext) {
    [super observeValueForKeyPath:keyPath
                         ofObject:object
                           change:change
                          context:context];
    return;
  }

  WKWebView* webView = (WKWebView*)object;
  if (webView.estimatedProgress == 1.0 &&
      [self.webViewTaskMap objectForKey:webView]) {
    [self delayedSnapshotForWebView:webView];
  }
}

- (void)delayedSnapshotForWebView:(WKWebView*)webView {
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                               (int64_t)(kSnapshotDelaySeconds * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   VivaldiThumbnailCapturerTask* task =
                       [self.webViewTaskMap objectForKey:webView];
                   if (!task) {
                     return;
                   }
                   [self takeSnapshotForWebView:webView];
                 });
}

- (void)takeSnapshotForWebView:(WKWebView*)webView {
  VivaldiThumbnailCapturerTask* task =
      [self.webViewTaskMap objectForKey:webView];
  if (!task) {
    return;
  }

  __weak __typeof(self) weakSelf = self;
  [webView
      takeSnapshotWithConfiguration:nil
                  completionHandler:^(UIImage* image, NSError* error) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                      __strong __typeof(weakSelf) strongSelf = weakSelf;
                      if (task.completion) {
                        task.completion(image, error);
                      }
                      if (!strongSelf) {
                        return;
                      }
                      [strongSelf.webViewTaskMap removeObjectForKey:webView];
                      [strongSelf.webViewPool addObject:webView];

                      if (strongSelf.taskQueue.count > 0) {
                        VivaldiThumbnailCapturerTask* nextTask =
                            strongSelf.taskQueue.firstObject;
                        [strongSelf.taskQueue removeObjectAtIndex:0];
                        [strongSelf captureSnapshotWithURL:nextTask.url
                                                completion:nextTask.completion];
                      }
                    });
                  }];
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction
                    decisionHandler:
                        (void (^)(WKNavigationActionPolicy))decisionHandler {
  decisionHandler(WKNavigationActionPolicyAllow);
}

@end
