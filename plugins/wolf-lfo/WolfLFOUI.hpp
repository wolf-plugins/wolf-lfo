#ifndef WOLF_LFO_UI_HPP_INCLUDED
#define WOLF_LFO_UI_HPP_INCLUDED

#include "WolfLFOParameters.hpp"
#include "DistrhoUI.hpp"
#include "GraphWidget.hpp"
#include "RemoveDCSwitch.hpp"
#include "ResetGraphButton.hpp"
#include "OversampleWheel.hpp"
#include "VolumeKnob.hpp"
#include "ResizeHandle.hpp"
#include "LabelBox.hpp"
#include "BipolarModeSwitch.hpp"
#include "GlowingLabelsBox.hpp"
#include "NanoLabel.hpp"
#include "WidgetBar.hpp"
#include "ArrowButton.hpp"
#include "LabelBoxList.hpp"

START_NAMESPACE_DISTRHO

class WolfLFOUI : public UI,
                     public NanoSwitch::Callback,
                     public NanoButton::Callback,
                     public NanoWheel::Callback,
                     public NanoKnob::Callback,
                     public ResizeHandle::Callback
{
public:
  WolfLFOUI();
  ~WolfLFOUI();

  float getParameterValue(uint32_t index) const;

protected:
  void parameterChanged(uint32_t, float value) override;
  void tryRememberSize();
  void positionWidgets(uint width, uint height);

  void nanoSwitchClicked(NanoSwitch *nanoSwitch);
  void nanoButtonClicked(NanoButton *nanoButton);
  void nanoWheelValueChanged(NanoWheel *nanoWheel, const int value);
  void nanoKnobValueChanged(NanoKnob *nanoKnob, const float value);

  void resizeHandleMoved(int width, int height);

  void stateChanged(const char *key, const char *value) override;
  void onNanoDisplay() override;
  void uiIdle() override;
  void uiReshape(uint width, uint height) override;
  bool onKeyboard(const KeyboardEvent &ev) override;
  bool onMouse(const MouseEvent &ev) override;

private:
  void toggleBottomBarVisibility();

  ScopedPointer<RemoveDCSwitch> fSwitchBPMSync;
  ScopedPointer<NanoLabel> fLabelBPMSync;

  ScopedPointer<VolumeKnob> fKnobPreGain, fKnobWet, fKnobPostGain;
  ScopedPointer<LabelBox> fLabelPreGain, fLabelWet, fLabelPostGain;

  ScopedPointer<VolumeKnob> fKnobHorizontalWarp;
  ScopedPointer<LabelBoxList> fLabelListHorizontalWarpType;

  ScopedPointer<ArrowButton> fButtonLeftArrowHorizontalWarp, fButtonRightArrowHorizontalWarp;

  ScopedPointer<ResizeHandle> fHandleResize;

  ScopedPointer<GraphWidget> fGraphWidget;
  ScopedPointer<WidgetBar> fGraphBar;
  ScopedPointer<ResetGraphButton> fButtonResetGraph;
  ScopedPointer<NanoLabel> fLabelButtonResetGraph;

  bool fBottomBarVisible;

  DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WolfLFOUI)
};

END_NAMESPACE_DISTRHO

#endif