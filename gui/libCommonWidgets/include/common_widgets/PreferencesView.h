/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "components/RaCoPreferences.h"

#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QWidget>
#include <utility>
#include <vector>

namespace raco::components {
class RaCoPreferences;
};

namespace raco::common_widgets {

class PreferencesView final : public QDialog {
	Q_OBJECT
public:
	explicit PreferencesView(QWidget* parent);

	bool dirty();

	Q_SLOT void save();
	Q_SIGNAL void dirtyChanged(bool dirty);

private:
	typedef QString raco::components::RaCoPreferences::*RaCoPreferencesQStringMember;
	std::vector<std::pair<RaCoPreferencesQStringMember, QLineEdit*>> stringEdits_;
};

}  // namespace raco::common_widgets
