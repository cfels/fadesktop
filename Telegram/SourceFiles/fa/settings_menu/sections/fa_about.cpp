/*
This file is part of FAgram Desktop,
the unofficial desktop client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/fagramdesktop/fadesktop/blob/dev/LEGAL
*/

#include "fa/settings_menu/sections/fa_about.h"
#include "fa/ui/md3/fa_cards.h"
#include "fa/ui/md3/svg_assets.h"
#include "fa/fa_version.h"
#include "fa_lang_auto.h"

#include "boxes/about_box.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "lang_auto.h"
#include "settings/settings_common.h"
#include "ui/basic_click_handlers.h"
#include "ui/effects/animation_value.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "ui/rp_widget.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>

namespace Settings {
namespace {

const auto kDonationUrl = u"https://burhanverse.eu.org/?section=support"_q;

void RenderSvgShape(
		QPainter &p,
		std::string_view svgData,
		const QRectF &rect,
		const QColor &color) {
	const auto colorName = color.name().toUtf8();
	auto data = QByteArray::fromRawData(svgData.data(), svgData.size()).trimmed();
	data.replace("#FFFFFF", colorName);
	data.replace("#ffffff", colorName);
	data.replace("#D0BCFF", colorName);
	data.replace("#d0bcff", colorName);
	data.replace("#E3E3E3", colorName);
	data.replace("#e3e3e3", colorName);

	auto renderer = QSvgRenderer(data);
	renderer.render(&p, rect);
}

void RenderFagramLogo(
		QPainter &p,
		const QRectF &rect,
		const QColor &accent,
		const QColor &bg) {
	const auto isDark = (bg.lightness() < 128);
	const auto cardBg = anim::color(accent, bg, 0.85);
	const auto accentSoft = isDark
		? cardBg
		: anim::color(accent, QColor(255, 255, 255), 0.45);
	const auto badgeStroke = isDark
		? accent
		: anim::color(accent, bg, 0.45);
	const auto badgeFill = isDark
		? anim::color(accent, bg, 0.90)
		: anim::color(accent, QColor(255, 255, 255), 0.96);

	auto data = QByteArray::fromRawData(
		fa::svg::fagram_logo.data(),
		fa::svg::fagram_logo.size()).trimmed();
	data.replace("#FF5858", accent.name().toUtf8());
	data.replace("#ff5858", accent.name().toUtf8());
	data.replace("#FFA3A3", accentSoft.name().toUtf8());
	data.replace("#ffa3a3", accentSoft.name().toUtf8());
	data.replace("#FFB2B2", badgeStroke.name().toUtf8());
	data.replace("#ffb2b2", badgeStroke.name().toUtf8());
	data.replace("#FFEDF4", badgeFill.name().toUtf8());
	data.replace("#ffedf4", badgeFill.name().toUtf8());

	auto renderer = QSvgRenderer(data);
	renderer.render(&p, rect);
}

class AboutLogoWidget final : public Ui::RpWidget {
public:
	explicit AboutLogoWidget(QWidget *parent) : RpWidget(parent) {
		resize(width(), 230);
	}

	int resizeGetHeight(int newWidth) override {
		return 230;
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		PainterHighQualityEnabler hq(p);

		const auto cx = width() / 2.0;
		const auto cy = height() / 2.0;

		const auto accent = st::windowActiveTextFg->c;
		const auto bg = st::windowBg->c;
		const auto isDark = (bg.lightness() < 128);

		constexpr auto outerSize = 176.0;
		const auto outerRect = QRectF(
			cx - outerSize / 2.0,
			cy - outerSize / 2.0,
			outerSize,
			outerSize);
		const auto outerColor = isDark
			? anim::color(accent, bg, 0.85)
			: anim::color(accent, bg, 0.78);
		RenderSvgShape(p, fa::svg::material_shape2, outerRect, outerColor);

		constexpr auto logoSize = 128.0;
		const auto logoRect = QRectF(
			cx - logoSize / 2.0,
			cy - logoSize / 2.0,
			logoSize,
			logoSize);
		RenderFagramLogo(p, logoRect, accent, bg);
	}
};

} // namespace

FAAbout::FAAbout(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent(controller);
}

rpl::producer<QString> FAAbout::title() {
	return tr::lng_menu_about();
}

void FAAbout::setupLogo(not_null<Ui::VerticalLayout*> container) {
	container->add(
		object_ptr<AboutLogoWidget>(container),
		style::margins(0, 8, 0, 8));
}

void FAAbout::setupAbout(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller) {
	FA::Ui::AddModernSectionHeader(container, tr::lng_menu_about());

	const auto card = FA::Ui::CreateCardContainer(container, 0, 8);

	const auto inner = card->add(
		object_ptr<Ui::VerticalLayout>(card),
		style::margins(18, 16, 18, 16));

	const auto appTitle = inner->add(
		object_ptr<Ui::FlatLabel>(
			inner,
			rpl::single(u"FAgram Desktop"_q),
			st::boxTitle),
		style::margins(0, 0, 0, 4));
	appTitle->setTextColorOverride(st::windowFg->c);

	const auto versionLabel = inner->add(
		object_ptr<Ui::FlatLabel>(
			inner,
			rpl::single(u"Version %1"_q.arg(currentVersionText())),
			st::boxDividerLabel),
		style::margins(0, 0, 0, 8));
	versionLabel->setTextColorOverride(st::windowSubTextFg->c);
	versionLabel->setClickHandlerFilter([=](const auto &...) {
		File::OpenUrl(Core::App().changelogLink());
		return true;
	});

	const auto descLabel = inner->add(
		object_ptr<Ui::FlatLabel>(
			inner,
			rpl::single(u"A feature rich unofficial desktop client based on Telegram Desktop with Material Design 3."_q),
			st::boxDividerLabel));
	descLabel->setTextColorOverride(st::windowSubTextFg->c);

	const auto donateCard = FA::Ui::CreateCardContainer(container, 0, 8);
	FA::Ui::AddCardButton(
		donateCard,
		rpl::single(u"Donate"_q),
		[=] { UrlClickHandler::Open(kDonationUrl); },
		nullptr,
		nullptr,
		false);
}

void FAAbout::setupLinks(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller) {
	FA::Ui::AddModernSectionHeader(container, fatr::fa_links());
	const auto card = FA::Ui::CreateCardContainer(container, 0, 8);

	FA::Ui::AddCardButton(
		card,
		fatr::fa_channel(),
		[=] { Core::App().openLocalUrl("tg://resolve?domain=FAgramDesktop", {}); },
		&st::menuIconChannel,
		rpl::single(u"@FAgramDesktop"_q),
		false);

	FA::Ui::AddCardDivider(card);

	FA::Ui::AddCardButton(
		card,
		fatr::fa_group(),
		[=] { Core::App().openLocalUrl("tg://resolve?domain=FAgramChat", {}); },
		&st::menuIconGroups,
		rpl::single(u"@FAgramChat"_q),
		false);

	FA::Ui::AddCardDivider(card);

	FA::Ui::AddCardButton(
		card,
		fatr::fa_translation(),
		[=] { UrlClickHandler::Open("https://hosted.weblate.org/projects/fagramdesktop/"); },
		&st::menuIconTranslate,
		rpl::single(u"Weblate"_q),
		false);

	FA::Ui::AddCardDivider(card);

	FA::Ui::AddCardButton(
		card,
		fatr::fa_source_code(),
		[=] { UrlClickHandler::Open("https://github.com/fagramdesktop/fadesktop"); },
		&st::menuIconLink,
		rpl::single(u"GitHub"_q),
		false);
}

void FAAbout::setupFooter(not_null<Ui::VerticalLayout*> container) {
	Ui::AddSkip(container, 8);
	const auto label = container->add(
		object_ptr<Ui::FlatLabel>(
			container,
			rpl::single(u"Made with \u2764 by Burhanverse & Contributors"_q),
			st::boxDividerLabel),
		style::margins(16, 8, 16, 24),
		style::al_top);
	label->setTextColorOverride(st::windowSubTextFg->c);
}

void FAAbout::setupContent(not_null<Window::SessionController*> controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	setupLogo(content);
	setupAbout(content, controller);
	setupLinks(content, controller);
	setupFooter(content);

	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
