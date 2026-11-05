// The UMG notices (WBP_UmgDialog*: "A device was disconnected..."): a UUmgDialog widget in the viewport. One per
// language is laid out in the tree and only the selected one is visible, so the visible text blocks are the message.
#include "screens/screen.h"
#include "screens/umg.h"
#include "screens/util.h"
#include "speech.h"

namespace p3a::screens {
using namespace ue;

class UmgDialogScreen : public Screen {
  Finder dialog_{"UmgDialog"};
  std::string text_;
  std::string read() {
    std::string s;
    for (auto& t : umg::texts(dialog_.get())) s += (s.empty() ? "" : " ") + t;
    return s;
  }
 public:
  const char* id() const override { return "umg_dialog"; }
  int priority() const override { return 100; }   // a system notice covers everything
  bool active() override { UObject* d = dialog_.get(); return d && umg::in_viewport(d) && umg::visible(d); }
  void enter() override { text_.clear(); }
  void tick() override {
    std::string s = read();
    if (s.empty() || s == text_) return;
    text_ = s;
    speech::speak(s, true);
  }
  std::string describe() override { return read(); }
};

std::unique_ptr<Screen> make_umg_dialog() { return std::make_unique<UmgDialogScreen>(); }
}  // namespace p3a::screens
