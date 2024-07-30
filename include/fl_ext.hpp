#ifndef FL_EXT_HPP
#define FL_EXT_HPP

#include "fltk.hpp"
#include "fl_flow.hpp"

#include <string>
#include <functional>

namespace fle
{
    using CallBack = std::function<void()>;

    inline const int BOX_COUNT = 6;
    enum Box
    {
        WIN10_BTN_UP = 0,
        WIN10_BTN_DOWN,
        WIN10_BTN_HOVER,
        WIN10_BTN_FRAME,
        WIN10_INPUT_IDLE,
        WIN10_INPUT_ACTIVE,

        WIN11_BTN_UP,
        WIN11_BTN_DOWN,
        WIN11_BTN_HOVER,
        WIN11_BTN_FRAME,
        WIN11_INPUT_IDLE,
        WIN11_INPUT_ACTIVE,
        WIN11_PROGRESS_BAR,
        WIN11_PROGRESS_FILL
    };

    class Color
    {
      private:
        unsigned char r_{255};
        unsigned char g_{255};
        unsigned char b_{255};

      public:
        inline Color();
        inline Color(unsigned char r, unsigned char g, unsigned char b);
        inline Color(unsigned int hex, bool index = false);

        inline operator Fl_Color();
        [[nodiscard]] inline int r() const { return r_; }
        [[nodiscard]] inline int g() const { return g_; }
        [[nodiscard]] inline int b() const { return b_; }
        inline void r(unsigned char r) { r_ = r; }
        inline void g(unsigned char g) { g_ = g; }
        inline void b(unsigned char b) { b_ = b; }

        inline void set(unsigned char r, unsigned char g, unsigned char b);
        inline void set(unsigned int hex);
        inline void index(unsigned int index);
    };

    inline void callBack_bridge(Fl_Widget* widget, void* func_ptr)
    {
        (*static_cast<CallBack*>(func_ptr))();
    }

    inline Fl_Boxtype box(int index)
    {
        return static_cast<Fl_Boxtype>(FL_FREE_BOXTYPE + index);
    }

    class InitializeWin10Style
    {
      public:
        inline InitializeWin10Style()
        {
            Fl::set_boxtype(
                box(WIN10_BTN_UP),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xE1E1E1));
                    fl_rect(x, y, w, h, Color(0xADADAD));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN10_BTN_DOWN),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xCCE4F7));
                    fl_rect(x, y, w, h, Color(0x005499));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN10_BTN_HOVER),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xE5F1FB));
                    fl_rect(x, y, w, h, Color(0x0078D7));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN10_BTN_FRAME),
                [](int x, int y, int w, int h, Fl_Color c) { fl_rect(x, y, w, h, Color(0x7A7A7A)); }, 0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN10_INPUT_IDLE),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0x7A7A7A));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN10_INPUT_ACTIVE),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0x0078D7));
                },
                0, 0, 0, 0);
        }
    };

    class InitializeWin11Style
    {
      public:
        inline InitializeWin11Style()
        {
            Fl::set_boxtype(
                box(WIN11_BTN_UP),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFDFDFD));
                    fl_rect(x, y, w, h, Color(0xD0D0D0));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_BTN_DOWN),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xCCE4F7));
                    fl_rect(x, y, w, h, Color(0x005499));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_BTN_HOVER),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xE0EEF9));
                    fl_rect(x, y, w, h, Color(0x006BBE));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_BTN_FRAME),
                [](int x, int y, int w, int h, Fl_Color c) { fl_rect(x, y, w, h, Color(0xECECEC)); }, 0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_INPUT_IDLE),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0xECECEC));
                    fl_rect(x, y + h - 1, w, 1, Color(0x838383));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_INPUT_ACTIVE),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0xECECEC));
                    fl_rect(x, y + h - 2, w, 2, Color(0x0067C0));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_PROGRESS_BAR),
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xBCBCBC));
                    fl_rect(x, y, w, h, Color(0xE6E6E6));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                box(WIN11_PROGRESS_FILL),
                [](int x, int y, int w, int h, Fl_Color c) { fl_rectf(x, y, w, h, Color(0x26A0DA)); }, 0, 0, 0, 0);
        }
    };

    // widget extensions
    template <typename Wd>
    class Attrib
    {
      private:
        Wd* widget_ = nullptr;

      public:
        inline Attrib(Wd* widget);
        inline Attrib(Wd& widget);

        // pointer to the widget itself
        inline Wd& widget() { return *widget_; }
        inline Wd* widget_ptr() { return widget_; }
        inline operator Wd&() { return *widget_; }
        inline operator Wd*() { return widget_; }

        inline bool operator==(Attrib<Wd>&& target);

        inline std::string label_a();
        inline void label_a(const std::string& text);
        inline std::string tooltip_a();
        inline void tooltip_a(const std::string& text);

        [[nodiscard]] inline CallBack* callback() const;
        inline void callback(CallBack* cb);
        inline void callback(CallBack& cb);
    };

    template <typename Wd_B>
    Attrib<Wd_B> make_attrib(Wd_B& widget)
    {
        return Attrib<Wd_B>(widget);
    }

    template <typename Wd_B>
    Attrib<Wd_B> make_attrib(Wd_B* widget)
    {
        return Attrib<Wd_B>(widget);
    }

    template <typename Wd = fl::Widget>
    class Ext : public Wd, public Attrib<Wd>
    {
      private:
        CallBack ext_cb_;

      public:
        template <typename... Args>
        constexpr inline Ext(Args&&... args);

        void callback(Fl_Callback) = delete;
        void default_callback() = delete;
        [[nodiscard]] inline CallBack* callback() const = delete;
        inline void callback(CallBack* cb) = delete;
        inline void callback(CallBack& cb) = delete;

        void callback(const CallBack& func);
        CallBack& callback();
    };

    template <typename Wd, typename... Args>
    inline constexpr Ext<Wd> make_ext(Args&&... args)
    {
        return Ext<Wd>(std::forward<Args>(args)...);
    }

    // implementations

    inline Color::Color(unsigned char r, unsigned char g, unsigned char b)
        : r_(r),
          g_(g),
          b_(b)
    {
    }

    inline Color::Color(unsigned int hex, bool index)
    {
        if (index)
        {
            Fl::get_color(hex, r_, g_, b_);
        }
        else
        {
            r_ = hex / 0x10000;
            g_ = (hex / 0x100) % 0x100;
            b_ = hex % 0x100;
        }
    }

    inline Color::operator Fl_Color()
    {
        return fl_rgb_color(r_, g_, b_);
    };

    inline void Color::set(unsigned char r, unsigned char g, unsigned char b)
    {
        r_ = r;
        g_ = g;
        b_ = b;
    }

    inline void Color::set(unsigned int hex)
    {
        r_ = hex / 0x10000;
        g_ = ((hex / 0x100) % 0x100);
        b_ = hex % 0x100;
    }

    inline void Color::index(unsigned int index)
    {
        Fl::get_color(index, r_, g_, b_);
    }

    //////////////////////////////Blank//////////////////////////////

    template <typename Wd_B>
    inline Attrib<Wd_B>::Attrib(Wd_B* widget)
        : widget_(widget)
    {
    }

    template <typename Wd_B>
    inline Attrib<Wd_B>::Attrib(Wd_B& widget)
        : widget_(&widget)
    {
    }

    template <typename Wd_B>
    inline bool Attrib<Wd_B>::operator==(Attrib<Wd_B>&& target)
    {
        return (target.widget_ == widget_) ? true : false;
    }

    // label and tooltip extensions
    template <typename Wd_B>
    inline std::string Attrib<Wd_B>::label_a()
    {
        return widget_->label();
    }

    template <typename Wd_B>
    inline void Attrib<Wd_B>::label_a(const std::string& text)
    {
        widget_->copy_label(text.c_str());
    }

    template <typename Wd_B>
    inline std::string Attrib<Wd_B>::tooltip_a()
    {
        return widget_->tooltip();
    }

    template <typename Wd_B>
    inline void Attrib<Wd_B>::tooltip_a(const std::string& text)
    {
        widget_->copy_tooltip(text.c_str());
    }

    // callback extension
    template <typename Wd_B>
    inline CallBack* Attrib<Wd_B>::callback() const
    {
        return static_cast<CallBack*>(widget_->user_data());
    }

    template <typename Wd_B>
    inline void Attrib<Wd_B>::callback(CallBack* cb)
    {
        widget_->callback(callBack_bridge, cb);
    }

    template <typename Wd_B>
    inline void Attrib<Wd_B>::callback(CallBack& cb)
    {
        widget_->callback(callBack_bridge, &cb);
    }

    template <typename Wd>
    template <typename... Args>
    constexpr inline Ext<Wd>::Ext(Args&&... args)
        : Wd(std::forward<Args>(args)...),
          Attrib<Wd>(this)
    {
    }

    template <typename Wd_T>
    inline void Ext<Wd_T>::callback(const CallBack& func)
    {
        ext_cb_ = func;
        Attrib<Wd_T>::callback(&ext_cb_);
    }

    template <typename Wd_T>
    inline CallBack& Ext<Wd_T>::callback()
    {
        return ext_cb_;
    }
}; // namespace fle

namespace fle
{
    using Flow = fl::Flow;
    using Global = Fl;
    using Group = Ext<Fl_Group>;
    using Adjuster = Ext<Fl_Adjuster>;
    using Bitmap = Ext<Fl_Bitmap>;
    using BMPImage = Ext<Fl_BMP_Image>;
    using Browser = Ext<Fl_Browser>;
    using Button = Ext<Fl_Button>;
    using Chart = Ext<Fl_Chart>;
    using CheckBrowser = Ext<Fl_Check_Browser>;
    using CheckButton = Ext<Fl_Check_Button>;
    using Choice = Ext<Fl_Choice>;
    using Clock = Ext<Fl_Clock>;
    using ColorChooser = Ext<Fl_Color_Chooser>;
    using CopySurface = Ext<Fl_Copy_Surface>;
    using Counter = Ext<Fl_Counter>;
    using Dial = Ext<Fl_Dial>;
    using DoubleWindow = Ext<Fl_Double_Window>;
    using FileBrowser = Ext<Fl_File_Browser>;
    using FileChooser = Ext<Fl_File_Chooser>;
    using FileIcon = Ext<Fl_File_Icon>;
    using FileInput = Ext<Fl_File_Input>;
    using FillDial = Ext<Fl_Fill_Dial>;
    using FillSlider = Ext<Fl_Fill_Slider>;
    using FloatInput = Ext<Fl_Float_Input>;
    using FormsBitmap = Ext<Fl_FormsBitmap>;
    using FormsPixmap = Ext<Fl_FormsPixmap>;
    using Free = Ext<Fl_Free>;
    using GIFImage = Ext<Fl_GIF_Image>;
    using GlWindow = Ext<Fl_Gl_Window>;
    using Group = Ext<Fl_Group>;
    using HelpDialog = Ext<Fl_Help_Dialog>;
    using HelpView = Ext<Fl_Help_View>;
    using HoldBrowser = Ext<Fl_Hold_Browser>;
    using HorFillSlider = Ext<Fl_Hor_Fill_Slider>;
    using HorNiceSlider = Ext<Fl_Hor_Nice_Slider>;
    using HorSlider = Ext<Fl_Hor_Slider>;
    using HorValueSlider = Ext<Fl_Hor_Value_Slider>;
    using ImageSurface = Ext<Fl_Image_Surface>;
    using Image = Ext<Fl_Image>;
    using InputChoice = Ext<Fl_Input_Choice>;
    using Input = Ext<Fl_Input>;
    using IntInput = Ext<Fl_Int_Input>;
    using JPEGImage = Ext<Fl_JPEG_Image>;
    using LightButton = Ext<Fl_Light_Button>;
    using LineDial = Ext<Fl_Line_Dial>;
    using MenuBar = Ext<Fl_Menu_Bar>;
    using MenuButton = Ext<Fl_Menu_Button>;
    using MenuItem = Ext<Fl_Menu_Item>;
    using MenuWindow = Ext<Fl_Menu_Window>;
    using Menu = Ext<Fl_Menu>;
    using MultiBrowser = Ext<Fl_Multi_Browser>;
    using MultiLabel = Ext<Fl_Multi_Label>;
    using MultilineInput = Ext<Fl_Multiline_Input>;
    using MultilineOutput = Ext<Fl_Multiline_Output>;
    using NativeFile_Chooser = Ext<Fl_Native_File_Chooser>;
    using NiceSlider = Ext<Fl_Nice_Slider>;
    using Widget = Ext<Fl_Widget>;
    using Output = Ext<Fl_Output>;
    using OverlayWindow = Ext<Fl_Overlay_Window>;
    using Pack = Ext<Fl_Pack>;
    using PagedDevice = Ext<Fl_Paged_Device>;
    using Pixmap = Ext<Fl_Pixmap>;
    using Plugin = Ext<Fl_Plugin>;
    using PNGImage = Ext<Fl_PNG_Image>;
    using PNMImage = Ext<Fl_PNM_Image>;
    using Positioner = Ext<Fl_Positioner>;
    using Preferences = Ext<Fl_Preferences>;
    using Printer = Ext<Fl_Printer>;
    using Progress = Ext<Fl_Progress>;
    using RadioButton = Ext<Fl_Radio_Button>;
    using RadioLight_Button = Ext<Fl_Radio_Light_Button>;
    using RadioRound_Button = Ext<Fl_Radio_Round_Button>;
    using RepeatButton = Ext<Fl_Repeat_Button>;
    using ReturnButton = Ext<Fl_Return_Button>;
    using RGBImage = Ext<Fl_RGB_Image>;
    using Roller = Ext<Fl_Roller>;
    using RoundButton = Ext<Fl_Round_Button>;
    using RoundClock = Ext<Fl_Round_Clock>;
    using Scroll = Ext<Fl_Scroll>;
    using Scrollbar = Ext<Fl_Scrollbar>;
    using SecretInput = Ext<Fl_Secret_Input>;
    using SelectBrowser = Ext<Fl_Select_Browser>;
    using SharedImage = Ext<Fl_Shared_Image>;
    using SimpleCounter = Ext<Fl_Simple_Counter>;
    using SingleWindow = Ext<Fl_Single_Window>;
    using Slider = Ext<Fl_Slider>;
    using Spinner = Ext<Fl_Spinner>;
    using SysMenu_ar = Ext<Fl_Sys_Menu_Bar>;
    using TableRow = Ext<Fl_Table_Row>;
    using Table = Ext<Fl_Table>;
    using Tabs = Ext<Fl_Tabs>;
    using TextBuffer = Ext<Fl_Text_Buffer>;
    using TextDisplay = Ext<Fl_Text_Display>;
    using TextEditor = Ext<Fl_Text_Editor>;
    using Tile = Ext<Fl_Tile>;
    using TiledImage = Ext<Fl_Tiled_Image>;
    using Timer = Ext<Fl_Timer>;
    using ToggleButton = Ext<Fl_Toggle_Button>;
    using Tooltip = Ext<Fl_Tooltip>;
    using TreeItem_Array = Ext<Fl_Tree_Item_Array>;
    using TreeItem = Ext<Fl_Tree_Item>;
    using TreePrefs = Ext<Fl_Tree_Prefs>;
    using Tree = Ext<Fl_Tree>;
    using Valuator = Ext<Fl_Valuator>;
    using ValueInput = Ext<Fl_Value_Input>;
    using ValueOutput = Ext<Fl_Value_Output>;
    using ValueSlider = Ext<Fl_Value_Slider>;
    using Widget = Ext<Fl_Widget>;
    using Window = Ext<Fl_Window>;
    using Wizard = Ext<Fl_Wizard>;
    using XBMImage = Ext<Fl_XBM_Image>;
    using XPMImage = Ext<Fl_XPM_Image>;
}; // namespace fle

#endif // FL_EXT_HPP