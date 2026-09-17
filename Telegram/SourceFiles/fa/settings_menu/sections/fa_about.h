/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fagramdesktop/fadesktop/blob/dev/LEGAL
*/

#pragma once

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"

namespace Window {
class SessionController;
} // namespace Window

namespace Settings {

class FAAbout : public Section<FAAbout> {
public:
	FAAbout(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);
	void setupLogo(not_null<Ui::VerticalLayout*> container);
	void setupAbout(not_null<Ui::VerticalLayout*> container, not_null<Window::SessionController*> controller);
	void setupLinks(not_null<Ui::VerticalLayout*> container, not_null<Window::SessionController*> controller);
	void setupFooter(not_null<Ui::VerticalLayout*> container);
};

} // namespace Settings
