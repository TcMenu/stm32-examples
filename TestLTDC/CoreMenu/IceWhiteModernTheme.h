#ifndef TCMENU_THEME_BLOCK
#define TCMENU_THEME_BLOCK

#include <graphics/TcThemeBuilder.h>

color_t defaultItemPalette[] = { RGB(46, 58, 69), RGB(247, 249, 251), RGB(107, 122, 136), RGB(174, 230, 248) };
color_t defaultTitlePalette[] = { RGB(46, 58, 69), RGB(74, 144, 226), RGB(107, 122, 136), RGB(174, 230, 248) };

/**
 * This is one of the stock themes, you can modify it to meet your requirements, and it will not be updated by tcMenu
 * Designer unless you delete it. This sets up the fonts, spacing and padding for all items.
 * @param gr the graphical renderer
 */
void applyTheme(GraphicsDeviceRenderer& gr) {

    // See https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/themes/rendering-with-themes-icons-grids/
    TcThemeBuilder themeBuilder(gr);
    themeBuilder.withSelectedColors(RGB(208, 232, 255), RGB(107, 122, 136))
            .dimensionsFromRenderer()
            .withItemPadding(MenuPadding(2))
            .withRenderingSettings(BaseGraphicalRenderer::TITLE_ALWAYS, false)
            .withPalette(defaultItemPalette)
            .withTcUnicodeFont(RobotoRegular16pt)
            .withSpacing(1)
            .withStandardLowResCursorIcons()
            .enableTcUnicode();

    themeBuilder.defaultTitleProperties()
            .withTcUnicodeFont(RobotoMedium24)
            .withPalette(defaultTitlePalette)
            .withPadding(MenuPadding(4))
            .withJustification(tcgfx::GridPosition::JUSTIFY_CENTER_WITH_VALUE)
            .withSpacing(2)
            .apply();

    themeBuilder.defaultActionProperties()
            .withJustification(tcgfx::GridPosition::JUSTIFY_CENTER_WITH_VALUE)
            .apply();

    themeBuilder.defaultItemProperties()
            .withJustification(tcgfx::GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT)
            .apply();

    themeBuilder.apply();
}

#endif //TCMENU_THEME_BLOCK

