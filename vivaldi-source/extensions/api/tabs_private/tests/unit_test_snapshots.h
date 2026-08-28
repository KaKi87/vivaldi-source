// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#include <set>
#include <string>

namespace vivaldi::test_snapshots {
std::set<std::string> NewTabAllStrategies();
std::set<std::string> PanelLinkAndConfigTabPosition();
std::set<std::string> InvokedByLinkClickSnapshot();
std::set<std::string> NewTabByCommandPosition();
std::set<std::string> TapFromSpeedDialPosition();
std::set<std::string> NewTopLevelTab();
std::set<std::string> MoveSnapshotBasic();
std::set<std::string> MoveSnapshotOn();
}  // namespace vivaldi::test_snapshots
