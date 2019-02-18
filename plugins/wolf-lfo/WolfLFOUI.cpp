#include "DistrhoUI.hpp"

#include "WolfLFOUI.hpp"
#include "Window.hpp"
#include "Config.hpp"
#include "Layout.hpp"
#include "Fonts/chivo_bold.hpp"

#include <string>

#if defined(DISTRHO_OS_WINDOWS)
#include "windows.h"
#endif

START_NAMESPACE_DISTRHO

WolfLFOUI::WolfLFOUI() : UI(611, 662),
                         fBottomBarVisible(true)
{
    const uint minWidth = 611;
    const uint minHeight = 438;

    const uint knobsLabelBoxWidth = 66;
    const uint knobsLabelBoxHeight = 21;

    loadSharedResources();

    using namespace WOLF_FONTS;
    NanoVG::FontId chivoBoldId = createFontFromMemory("chivo_bold", (const uchar *)chivo_bold, chivo_bold_size, 0);
    NanoVG::FontId dejaVuSansId = findFont(NANOVG_DEJAVU_SANS_TTF);

    WolfLFOConfig::load();

    tryRememberSize();
    getParentWindow().saveSizeAtExit(true);

    const float width = getWidth();
    const float height = getHeight();

    fGraphWidget = new GraphWidget(this, Size<uint>(width - 4 * 2, height - 4 * 2 - 122));

    const float graphBarHeight = 42;

    fGraphBar = new WidgetBar(this, Size<uint>(width, graphBarHeight));
    fGraphBar->setStrokePaint(linearGradient(0, 0, 0, graphBarHeight, Color(43, 43, 43, 255), Color(34, 34, 34, 255)));
    fGraphBar->setStrokeWidth(4.0f);

    fSwitchBPMSync = new RemoveDCSwitch(this, Size<uint>(30, 29));
    fSwitchBPMSync->setCallback(this);
    fSwitchBPMSync->setId(paramBPMSync);

    fLabelBPMSync = new NanoLabel(this, Size<uint>(100, 29));
    fLabelBPMSync->setText("BPM SYNC");
    fLabelBPMSync->setFontId(chivoBoldId);
    fLabelBPMSync->setFontSize(14.0f);
    fLabelBPMSync->setAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    fLabelBPMSync->setMargin(Margin(3, 0, fSwitchBPMSync->getWidth() / 2.0f, 0));

    fLabelPreGain = new LabelBox(this, Size<uint>(knobsLabelBoxWidth, knobsLabelBoxHeight));
    fLabelPreGain->setText("PRE");

    fKnobPreGain = new VolumeKnob(this, Size<uint>(54, 54));
    fKnobPreGain->setCallback(this);
    fKnobPreGain->setRange(0.0f, 2.0f);
    fKnobPreGain->setId(paramPreGain);
    fKnobPreGain->setColor(Color(255, 197, 246, 255));

    fLabelWet = new LabelBox(this, Size<uint>(knobsLabelBoxWidth, knobsLabelBoxHeight));
    fLabelWet->setText("WET");

    fKnobWet = new VolumeKnob(this, Size<uint>(54, 54));
    fKnobWet->setCallback(this);
    fKnobWet->setRange(0.0f, 1.0f);
    fKnobWet->setId(paramWet);
    fKnobWet->setColor(Color(136, 228, 255));

    fLabelPostGain = new LabelBox(this, Size<uint>(knobsLabelBoxWidth, knobsLabelBoxHeight));
    fLabelPostGain->setText("POST");

    fKnobPostGain = new VolumeKnob(this, Size<uint>(54, 54));
    fKnobPostGain->setCallback(this);
    fKnobPostGain->setRange(0.0f, 1.0f);
    fKnobPostGain->setId(paramPostGain);
    fKnobPostGain->setColor(Color(143, 255, 147, 255));

    fKnobHorizontalWarp = new VolumeKnob(this, Size<uint>(54, 54));
    fKnobHorizontalWarp->setCallback(this);
    fKnobHorizontalWarp->setRange(0.0f, 1.0f);
    fKnobHorizontalWarp->setId(paramHorizontalWarpAmount);
    fKnobHorizontalWarp->setColor(Color(255, 225, 169, 255));

    fLabelListHorizontalWarpType = new LabelBoxList(this, Size<uint>(knobsLabelBoxWidth + 3, knobsLabelBoxHeight));
    fLabelListHorizontalWarpType->setLabels({"–", "BEND +", "BEND -", "BEND +/-", "SKEW +", "SKEW -", "SKEW +/-"});

    fButtonLeftArrowHorizontalWarp = new ArrowButton(this, Size<uint>(knobsLabelBoxHeight, knobsLabelBoxHeight));
    fButtonLeftArrowHorizontalWarp->setCallback(this);
    fButtonLeftArrowHorizontalWarp->setId(paramHorizontalWarpType);
    fButtonLeftArrowHorizontalWarp->setArrowDirection(ArrowButton::Left);

    fButtonRightArrowHorizontalWarp = new ArrowButton(this, Size<uint>(knobsLabelBoxHeight, knobsLabelBoxHeight));
    fButtonRightArrowHorizontalWarp->setCallback(this);
    fButtonRightArrowHorizontalWarp->setId(paramHorizontalWarpType);
    fButtonRightArrowHorizontalWarp->setArrowDirection(ArrowButton::Right);

    fHandleResize = new ResizeHandle(this, Size<uint>(18, 18));
    fHandleResize->setCallback(this);
    fHandleResize->setMinSize(minWidth, minHeight);

    fButtonResetGraph = new ResetGraphButton(this, Size<uint>(32, 32));
    fButtonResetGraph->setCallback(this);

    fLabelButtonResetGraph = new NanoLabel(this, Size<uint>(50, fButtonResetGraph->getHeight()));
    fLabelButtonResetGraph->setText("RESET");
    fLabelButtonResetGraph->setFontId(dejaVuSansId);
    fLabelButtonResetGraph->setFontSize(15.0f);
    fLabelButtonResetGraph->setAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    fLabelButtonResetGraph->setMargin(Margin(6, 0, std::round(fButtonResetGraph->getHeight() / 2.0f) + 1, 0));

    positionWidgets(width, height);
}

WolfLFOUI::~WolfLFOUI()
{
}

void WolfLFOUI::tryRememberSize()
{
    int width, height;
    FILE *file;
    std::string tmpFileName = PLUGIN_NAME ".tmp";

#if defined(DISTRHO_OS_WINDOWS)
    CHAR tempPath[MAX_PATH + 1];

    GetTempPath(MAX_PATH + 1, tempPath);
    std::string path = std::string(tempPath) + tmpFileName;
    file = fopen(path.c_str(), "r");
#else
    file = fopen(("/tmp/" + tmpFileName).c_str(), "r");
#endif

    if (file == NULL)
        return;

    const int numberScanned = fscanf(file, "%d %d", &width, &height);

    if (numberScanned == 2 && width && height)
    {
        setSize(width, height);
    }

    fclose(file);
}

void WolfLFOUI::positionWidgets(uint width, uint height)
{
    //TODO: Clean that up

    const float graphMargin = 8;
    const float bottomBarSize = fBottomBarVisible ? 102 : 0;
    const float graphBarHeight = fGraphBar->getHeight();
    const float graphBarMargin = 6;

    fGraphWidget->setSize(width - graphMargin * 2, height - graphMargin * 2 - bottomBarSize - graphBarHeight);
    fGraphWidget->setAbsolutePos(graphMargin, graphMargin);

    const float graphBottom = fGraphWidget->getAbsoluteY() + fGraphWidget->getHeight();

    fGraphBar->setWidth(width);
    fGraphBar->setAbsolutePos(0, graphBottom + graphBarMargin);
    fGraphBar->setFillPaint(radialGradient(width / 2.0f, graphBarHeight / 2.0f, graphBarHeight, width / 2.0f, Color(71, 74, 80, 255), Color(40, 42, 46, 255)));

    const float knobLabelMarginBottom = 12;

    fSwitchBPMSync->setAbsolutePos(24, height - 38);
    fLabelBPMSync->setAbsolutePos(24 + fSwitchBPMSync->getWidth(), height - 38);

    const float graphBarMiddleY = fGraphBar->getAbsoluteY() + fGraphBar->getHeight() / 2.0f;

    fButtonResetGraph->setAbsolutePos(20, graphBarMiddleY - fButtonResetGraph->getHeight() / 2.0f);
    fLabelButtonResetGraph->setAbsolutePos(fButtonResetGraph->getAbsoluteX() + fButtonResetGraph->getWidth(), fButtonResetGraph->getAbsoluteY());

    float centerAlignDifference = (fLabelPreGain->getWidth() - fKnobPreGain->getWidth()) / 2.0f;

    fKnobPreGain->setAbsolutePos(width - 225, height - 90);
    fLabelPreGain->setAbsolutePos(width - 225 - centerAlignDifference, height - fLabelPreGain->getHeight() - knobLabelMarginBottom);

    centerAlignDifference = (fLabelWet->getWidth() - fKnobWet->getWidth()) / 2.0f;

    fKnobWet->setAbsolutePos(width - 155, height - 90);
    fLabelWet->setAbsolutePos(width - 155 - centerAlignDifference, height - fLabelPreGain->getHeight() - knobLabelMarginBottom);

    centerAlignDifference = (fLabelPostGain->getWidth() - fKnobPostGain->getWidth()) / 2.0f;

    fKnobPostGain->setAbsolutePos(width - 85, height - 90);
    fLabelPostGain->setAbsolutePos(width - 85 - centerAlignDifference, height - fLabelPreGain->getHeight() - knobLabelMarginBottom);

    centerAlignDifference = (fLabelListHorizontalWarpType->getWidth() - fKnobHorizontalWarp->getWidth()) / 2.0f;

    fKnobHorizontalWarp->setAbsolutePos(fKnobPreGain->getAbsoluteX() - 230, height - 90);
    fLabelListHorizontalWarpType->setAbsolutePos(fKnobPreGain->getAbsoluteX() - 230 - centerAlignDifference, height - fLabelListHorizontalWarpType->getHeight() - knobLabelMarginBottom);

    fButtonLeftArrowHorizontalWarp->setAbsolutePos(fLabelListHorizontalWarpType->getAbsoluteX() - fButtonLeftArrowHorizontalWarp->getWidth(), fLabelListHorizontalWarpType->getAbsoluteY());
    fButtonRightArrowHorizontalWarp->setAbsolutePos(fLabelListHorizontalWarpType->getAbsoluteX() + fLabelListHorizontalWarpType->getWidth(), fLabelListHorizontalWarpType->getAbsoluteY());

    fHandleResize->setAbsolutePos(width - fHandleResize->getWidth(), height - fHandleResize->getHeight());
}

void WolfLFOUI::parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case paramPreGain:
        fKnobPreGain->setValue(value);
        break;
    case paramWet:
        fKnobWet->setValue(value);
        break;
    case paramPostGain:
        fKnobPostGain->setValue(value);
        break;
    case paramBPMSync:
        fSwitchBPMSync->setDown(value >= 0.50f);
        break;
    case paramHorizontalWarpType:
    {
        const int warpType = std::round(value);

        fGraphWidget->setHorizontalWarpType((wolf::WarpType)warpType);
        fLabelListHorizontalWarpType->setSelectedIndex(warpType);

        break;
    }
    case paramHorizontalWarpAmount:
        fKnobHorizontalWarp->setValue(value);
        fGraphWidget->setHorizontalWarpAmount(value);
        break;
    case paramPlayheadPos:
        fGraphWidget->updateInput(value);
        break;
    default:
        break;
    }
}

void WolfLFOUI::stateChanged(const char *key, const char *value)
{
    if (std::strcmp(key, "graph") == 0)
        fGraphWidget->rebuildFromString(value);

    repaint();
}

void WolfLFOUI::onNanoDisplay()
{
    const float width = getWidth();
    const float height = getHeight();

    //background
    beginPath();

    rect(0.f, 0.f, width, height);
    fillColor(WolfLFOConfig::plugin_background);

    fill();

    closePath();

    //shadow at the bottom of the graph
    beginPath();

    const int shadowHeight = 8;
    const int shadowMargin = 2;

    const float graphBottom = fGraphWidget->getAbsoluteY() + fGraphWidget->getHeight();
    const float shadowTop = graphBottom + shadowMargin;
    const float shadowBottom = shadowTop + shadowHeight;

    rect(0.0f, shadowTop, getWidth(), shadowHeight);

    Paint gradient = linearGradient(0, shadowTop, 0, shadowBottom, Color(21, 22, 30, 0), Color(21, 22, 30, 180));
    fillPaint(gradient);

    fill();

    closePath();
}

void WolfLFOUI::uiIdle()
{
}

bool WolfLFOUI::onMouse(const MouseEvent &ev)
{
    WolfLFOConfig::load();

    return false;
}

void WolfLFOUI::uiReshape(uint width, uint height)
{
    //setSize(width, height);
    positionWidgets(width, height);
}

bool WolfLFOUI::onKeyboard(const KeyboardEvent &ev)
{
    return true;
}

void WolfLFOUI::nanoSwitchClicked(NanoSwitch *nanoSwitch)
{
    const uint switchId = nanoSwitch->getId();
    const int value = nanoSwitch->isDown() ? 1 : 0;

    setParameterValue(switchId, value);
}

void WolfLFOUI::nanoButtonClicked(NanoButton *nanoButton)
{
    if (nanoButton == fButtonResetGraph)
    {
        fGraphWidget->reset();
        return;
    }

    if (nanoButton == fButtonLeftArrowHorizontalWarp)
    {
        fLabelListHorizontalWarpType->goPrevious();
    }
    else if (nanoButton == fButtonRightArrowHorizontalWarp)
    {
        fLabelListHorizontalWarpType->goNext();
    }

    const int index = fLabelListHorizontalWarpType->getSelectedIndex();

    setParameterValue(paramHorizontalWarpType, index);
    fGraphWidget->setHorizontalWarpType((wolf::WarpType)index);
}

void WolfLFOUI::nanoWheelValueChanged(NanoWheel *nanoWheel, const int value)
{
    const uint id = nanoWheel->getId();
}

void WolfLFOUI::nanoKnobValueChanged(NanoKnob *nanoKnob, const float value)
{
    const uint id = nanoKnob->getId();

    setParameterValue(id, value);

    if (id == paramHorizontalWarpAmount)
    {
        fGraphWidget->setHorizontalWarpAmount(value);
    }
}

void WolfLFOUI::resizeHandleMoved(int width, int height)
{
    setSize(width, height);
}

UI *createUI()
{
    return new WolfLFOUI();
}

END_NAMESPACE_DISTRHO