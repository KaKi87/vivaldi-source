#include "components/ext_data/tab_ext_data.h"

#include "components/ext_data/tab_ext_data_impl.h"

namespace vivaldi {

TabExtData::~TabExtData() {}

/*static*/ TabExtData* TabExtData::Get(content::WebContents* contents) {
  auto* res = TabExtDataImpl::FromWebContents(contents);
  CHECK(res);
  return res;
}

/*static*/ const TabExtData* TabExtData::Get(
    const content::WebContents* contents) {
  auto* res = TabExtDataImpl::FromWebContents(contents);
  CHECK(res);
  return res;
}

bool TabExtData::Has(const content::WebContents* contents) {
  if (!contents)
    return false;
  if (TabExtDataImpl::FromWebContents(contents))
    return true;
  return false;
}

TabExtData::Observer::~Observer() = default;

}  // namespace vivaldi
