#ifndef FL_EXT_HPP
#define FL_EXT_HPP

#include "fltk.hpp"
#include "fl_flow.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <functional>

#include <stb/stb_image.h>

#define STYLED_UP_BOX            FL_UP_BOX
#define STYLED_DOWN_BOX          FL_DOWN_BOX
#define STYLED_HOVER_BOX         FL_UP_FRAME
#define STYLED_INPUT_IDLE_BOX    FL_DOWN_FRAME
#define STYLED_INPUT_ACTIVE_BOX  FL_THIN_UP_BOX
#define STYLED_PROGRESS_BOX      FL_THIN_DOWN_BOX
#define STYLED_PROGRESS_FILL_BOX FL_THIN_UP_FRAME

namespace fle
{
    using CallBack = std::function<void()>;

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

    inline static double scale_factor_ = 1.0;
    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    class Ext : public Wd
    {
      private:
        CallBack ext_cb_ = []() {};

      public:
        template <typename... Args>
        constexpr inline Ext(Args&&... args);

        template <typename... Args>
            requires std::derived_from<Wd, fl::Group>
        constexpr inline Ext(Args&&... args);

        template <typename... Args>
            requires std::derived_from<Wd, fl::Input>
        constexpr Ext(Args&&... args);

        template <typename... Args>
            requires std::derived_from<Wd, fl::Slider>
        constexpr Ext(Args&&... args);

        void callback(Fl_Callback) = delete;
        void default_callback() = delete;
        [[nodiscard]] inline CallBack* callback() const = delete;
        inline void callback(CallBack* cb) = delete;
        inline void callback(CallBack& cb) = delete;

        void callback(const CallBack& func);
        CallBack& callback();
        CallBack clear_callback();
    };

    class InitializeWin11Style
    {
      public:
        inline InitializeWin11Style()
        {
            Fl::set_boxtype(
                STYLED_UP_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFDFDFD));
                    fl_rect(x, y, w, h, Color(0xD0D0D0));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_DOWN_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xCCE4F7));
                    fl_rect(x, y, w, h, Color(0x005499));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_HOVER_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xE0EEF9));
                    fl_rect(x, y, w, h, Color(0x006BBE));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_INPUT_IDLE_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0xECECEC));
                    fl_rect(x, y + h - 1, w, 1, Color(0x838383));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_INPUT_ACTIVE_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xFFFFFF));
                    fl_rect(x, y, w, h, Color(0xECECEC));
                    fl_rect(x, y + h - 2, w, 2, Color(0x0067C0));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_PROGRESS_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0xBCBCBC));
                    fl_rect(x, y, w, h, Color(0xE6E6E6));
                },
                0, 0, 0, 0);
            Fl::set_boxtype(
                STYLED_PROGRESS_FILL_BOX,
                [](int x, int y, int w, int h, Fl_Color c)
                {
                    fl_rectf(x, y, w, h, Color(0x26A0DA));
                    fl_rect(x, y, w, h, Color(0xE6E6E6));
                },
                0, 0, 0, 0);
        }
    };

    namespace literals
    {
        inline int operator""_px(unsigned long long integer)
        {
            return integer * scale_factor_;
        }
    }; // namespace literals

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

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    template <typename... Args>
    constexpr inline Ext<Wd>::Ext(Args&&... args)
        : Wd(std::forward<Args>(args)...)
    {
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    template <typename... Args>
        requires std::derived_from<Wd, fl::Group>
    constexpr inline Ext<Wd>::Ext(Args&&... args)
        : Wd(std::forward<Args>(args)...)
    {
        this->end();
        this->box(FL_NO_BOX);
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    template <typename... Args>
        requires std::derived_from<Wd, fl::Input>
    constexpr inline Ext<Wd>::Ext(Args&&... args)
        : Wd(std::forward<Args>(args)...)
    {
        this->box(STYLED_INPUT_ACTIVE_BOX);
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    template <typename... Args>
        requires std::derived_from<Wd, fl::Slider>
    constexpr inline Ext<Wd>::Ext(Args&&... args)
        : Wd(std::forward<Args>(args)...)
    {
        this->box(STYLED_PROGRESS_BOX);
        this->slider(STYLED_PROGRESS_FILL_BOX);
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    inline void Ext<Wd>::callback(const CallBack& func)
    {
        ext_cb_ = func;
        static_cast<Wd*>(this)->callback(callBack_bridge, &ext_cb_);
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    inline CallBack Ext<Wd>::clear_callback()
    {
        static_cast<Wd*>(this)->callback((Fl_Callback*)nullptr);
        return ext_cb_;
    }

    template <typename Wd>
        requires std::derived_from<Wd, fl::Widget>
    inline CallBack& Ext<Wd>::callback()
    {
        return ext_cb_;
    }
}; // namespace fle

namespace fle
{
    using Global = Fl;
    using Flow = Ext<fl::Flow>;
    using Group = Ext<Fl_Group>;
    using Adjuster = Ext<Fl_Adjuster>;
    using Bitmap = Fl_Bitmap;
    using BMPImage = Fl_BMP_Image;
    using Browser = Ext<Fl_Browser>;
    using Button = Ext<Fl_Button>;
    using Chart = Ext<Fl_Chart>;
    using CheckBrowser = Ext<Fl_Check_Browser>;
    using CheckButton = Ext<Fl_Check_Button>;
    using Choice = Ext<Fl_Choice>;
    using Clock = Ext<Fl_Clock>;
    using ColorChooser = Ext<Fl_Color_Chooser>;
    using CopySurface = Fl_Copy_Surface;
    using Counter = Ext<Fl_Counter>;
    using Dial = Ext<Fl_Dial>;
    using DoubleWindow = Ext<Fl_Double_Window>;
    using FileBrowser = Ext<Fl_File_Browser>;
    using FileChooser = Fl_File_Chooser;
    using FileIcon = Fl_File_Icon;
    using FileInput = Ext<Fl_File_Input>;
    using FillDial = Ext<Fl_Fill_Dial>;
    using FillSlider = Ext<Fl_Fill_Slider>;
    using FloatInput = Ext<Fl_Float_Input>;
    using FormsBitmap = Ext<Fl_FormsBitmap>;
    using FormsPixmap = Ext<Fl_FormsPixmap>;
    using Free = Ext<Fl_Free>;
    using GIFImage = Fl_GIF_Image;
    using GlWindow = Ext<Fl_Gl_Window>;
    using Group = Ext<Fl_Group>;
    using HelpDialog = Fl_Help_Dialog;
    using HelpView = Ext<Fl_Help_View>;
    using HoldBrowser = Ext<Fl_Hold_Browser>;
    using HorFillSlider = Ext<Fl_Hor_Fill_Slider>;
    using HorNiceSlider = Ext<Fl_Hor_Nice_Slider>;
    using HorSlider = Ext<Fl_Hor_Slider>;
    using HorValueSlider = Ext<Fl_Hor_Value_Slider>;
    using ImageSurface = Fl_Image_Surface;
    using Image = Fl_Image;
    using InputChoice = Ext<Fl_Input_Choice>;
    using Input = Ext<Fl_Input>;
    using IntInput = Ext<Fl_Int_Input>;
    using JPEGImage = Fl_JPEG_Image;
    using LightButton = Ext<Fl_Light_Button>;
    using LineDial = Ext<Fl_Line_Dial>;
    using MenuBar = Ext<Fl_Menu_Bar>;
    using MenuButton = Ext<Fl_Menu_Button>;
    using MenuItem = Fl_Menu_Item;
    using MenuWindow = Ext<Fl_Menu_Window>;
    using Menu = Fl_Menu;
    using MultiBrowser = Ext<Fl_Multi_Browser>;
    using MultiLabel = Fl_Multi_Label;
    using MultilineInput = Ext<Fl_Multiline_Input>;
    using MultilineOutput = Ext<Fl_Multiline_Output>;
    using NativeFile_Chooser = Fl_Native_File_Chooser;
    using NiceSlider = Ext<Fl_Nice_Slider>;
    using Widget = Ext<Fl_Widget>;
    using Output = Ext<Fl_Output>;
    using OverlayWindow = Ext<Fl_Overlay_Window>;
    using Pack = Ext<Fl_Pack>;
    using PagedDevice = Fl_Paged_Device;
    using Pixmap = Fl_Pixmap;
    using Plugin = Fl_Plugin;
    using PNGImage = Fl_PNG_Image;
    using PNMImage = Fl_PNM_Image;
    using Positioner = Fl_Positioner;
    using Preferences = Fl_Preferences;
    using Printer = Fl_Printer;
    using Progress = Ext<Fl_Progress>;
    using RadioButton = Ext<Fl_Radio_Button>;
    using RadioLight_Button = Ext<Fl_Radio_Light_Button>;
    using RadioRound_Button = Ext<Fl_Radio_Round_Button>;
    using RepeatButton = Ext<Fl_Repeat_Button>;
    using ReturnButton = Ext<Fl_Return_Button>;
    using RGBImage = Fl_RGB_Image;
    using Roller = Ext<Fl_Roller>;
    using RoundButton = Ext<Fl_Round_Button>;
    using RoundClock = Ext<Fl_Round_Clock>;
    using Scroll = Ext<Fl_Scroll>;
    using Scrollbar = Ext<Fl_Scrollbar>;
    using SecretInput = Ext<Fl_Secret_Input>;
    using SelectBrowser = Ext<Fl_Select_Browser>;
    using SharedImage = Fl_Shared_Image;
    using SimpleCounter = Ext<Fl_Simple_Counter>;
    using SingleWindow = Ext<Fl_Single_Window>;
    using Slider = Ext<Fl_Slider>;
    using Spinner = Ext<Fl_Spinner>;
    using SysMenu_ar = Ext<Fl_Sys_Menu_Bar>;
    using TableRow = Ext<Fl_Table_Row>;
    using Table = Ext<Fl_Table>;
    using Tabs = Ext<Fl_Tabs>;
    using TextBuffer = Fl_Text_Buffer;
    using TextDisplay = Ext<Fl_Text_Display>;
    using TextEditor = Ext<Fl_Text_Editor>;
    using Tile = Ext<Fl_Tile>;
    using TiledImage = Fl_Tiled_Image;
    using Timer = Ext<Fl_Timer>;
    using ToggleButton = Ext<Fl_Toggle_Button>;
    using Tooltip = Fl_Tooltip;
    using TreeItem_Array = Fl_Tree_Item_Array;
    using TreeItem = Fl_Tree_Item;
    using TreePrefs = Fl_Tree_Prefs;
    using Tree = Ext<Fl_Tree>;
    using Valuator = Ext<Fl_Valuator>;
    using ValueInput = Ext<Fl_Value_Input>;
    using ValueOutput = Ext<Fl_Value_Output>;
    using ValueSlider = Ext<Fl_Value_Slider>;
    using Widget = Ext<Fl_Widget>;
    using Window = Ext<Fl_Window>;
    using Wizard = Ext<Fl_Wizard>;
    using XBMImage = Fl_XBM_Image;
    using XPMImage = Fl_XPM_Image;
}; // namespace fle

namespace fle
{
    class Image2
    {
      private:
        std::unique_ptr<fl::Image> image_{};

      public:
        struct
        {
            int w_ = 0;
            int h_ = 0;
            int c_ = 0;
        } extent_;

        Image2(const std::filesystem::path& img_path, int w, int h)
        {
            unsigned char* pixels = stbi_load(img_path.generic_string().c_str(), //
                                              &extent_.w_, &extent_.h_, &extent_.c_, STBI_rgb_alpha);
            fle::RGBImage::RGB_scaling(FL_RGB_SCALING_BILINEAR);
            fle::RGBImage tmp_image(pixels, extent_.w_, extent_.h_, 4);
            image_.reset(tmp_image.copy(w, h));
            stbi_image_free(pixels);
        };

        operator Fl_Image&() { return *image_; };
        fl::Image* operator->() { return image_.get(); };

        void resize(int w, int h)
        {
            image_.reset(image_->copy(w, h));
        }
    };
}; // namespace fle

#endif // FL_EXT_HPP