#ifndef FLTK_HPP
#define FLTK_HPP

#include "FL/Fl_Adjuster.H"
#include "FL/fl_ask.H"
#include "FL/Fl_Bitmap.H"
#include "FL/Fl_BMP_Image.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Browser_.H"
#include "FL/Fl_Browser.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Cairo_Window.H"
#include "FL/Fl_Cairo.H"
#include "FL/Fl_Chart.H"
#include "FL/Fl_Check_Browser.H"
#include "FL/Fl_Check_Button.H"
#include "FL/Fl_Choice.H"
#include "FL/Fl_Clock.H"
#include "FL/Fl_Color_Chooser.H"
#include "FL/Fl_Copy_Surface.H"
#include "FL/Fl_Counter.H"
#include "FL/Fl_Device.H"
#include "FL/Fl_Dial.H"
#include "FL/Fl_Double_Window.H"
#include "FL/fl_draw.H"
#include "FL/Fl_Export.H"
#include "FL/Fl_File_Browser.H"
#include "FL/Fl_File_Chooser.H"
#include "FL/Fl_File_Icon.H"
#include "FL/Fl_File_Input.H"
#include "FL/Fl_Fill_Dial.H"
#include "FL/Fl_Fill_Slider.H"
#include "FL/Fl_Float_Input.H"
#include "FL/Fl_FormsBitmap.H"
#include "FL/Fl_FormsPixmap.H"
#include "FL/Fl_Free.H"
#include "FL/Fl_GIF_Image.H"
#include "FL/Fl_Gl_Window.H"
#include "FL/Fl_Group.H"
#include "FL/Fl_Help_Dialog.H"
#include "FL/Fl_Help_View.H"
#include "FL/Fl_Hold_Browser.H"
#include "FL/Fl_Hor_Fill_Slider.H"
#include "FL/Fl_Hor_Nice_Slider.H"
#include "FL/Fl_Hor_Slider.H"
#include "FL/Fl_Hor_Value_Slider.H"
#include "FL/Fl_Image_Surface.H"
#include "FL/Fl_Image.H"
#include "FL/Fl_Input_.H"
#include "FL/Fl_Input_Choice.H"
#include "FL/Fl_Input.H"
#include "FL/Fl_Int_Input.H"
#include "FL/Fl_JPEG_Image.H"
#include "FL/Fl_Light_Button.H"
#include "FL/Fl_Line_Dial.H"
#include "FL/Fl_Menu_.H"
#include "FL/Fl_Menu_Bar.H"
#include "FL/Fl_Menu_Button.H"
#include "FL/Fl_Menu_Item.H"
#include "FL/Fl_Menu_Window.H"
#include "FL/Fl_Menu.H"
#include "FL/fl_message.H"
#include "FL/Fl_Multi_Browser.H"
#include "FL/Fl_Multi_Label.H"
#include "FL/Fl_Multiline_Input.H"
#include "FL/Fl_Multiline_Output.H"
#include "FL/Fl_Native_File_Chooser.H"
#include "FL/Fl_Nice_Slider.H"
#include "FL/Fl_Object.H"
#include "FL/Fl_Output.H"
#include "FL/Fl_Overlay_Window.H"
#include "FL/Fl_Pack.H"
#include "FL/Fl_Paged_Device.H"
#include "FL/Fl_Pixmap.H"
#include "FL/Fl_Plugin.H"
#include "FL/Fl_PNG_Image.H"
#include "FL/Fl_PNM_Image.H"
#include "FL/Fl_Positioner.H"
#include "FL/Fl_PostScript.H"
#include "FL/Fl_Preferences.H"
#include "FL/Fl_Printer.H"
#include "FL/Fl_Progress.H"
#include "FL/Fl_Radio_Button.H"
#include "FL/Fl_Radio_Light_Button.H"
#include "FL/Fl_Radio_Round_Button.H"
#include "FL/Fl_Repeat_Button.H"
#include "FL/Fl_Return_Button.H"
#include "FL/Fl_RGB_Image.H"
#include "FL/Fl_Roller.H"
#include "FL/Fl_Round_Button.H"
#include "FL/Fl_Round_Clock.H"
#include "FL/Fl_Scroll.H"
#include "FL/Fl_Scrollbar.H"
#include "FL/Fl_Secret_Input.H"
#include "FL/Fl_Select_Browser.H"
#include "FL/Fl_Shared_Image.H"
#include "FL/fl_show_colormap.H"
#include "FL/fl_show_input.H"
#include "FL/Fl_Simple_Counter.H"
#include "FL/Fl_Single_Window.H"
#include "FL/Fl_Slider.H"
#include "FL/Fl_Spinner.H"
#include "FL/Fl_Sys_Menu_Bar.H"
#include "FL/Fl_Table_Row.H"
#include "FL/Fl_Table.H"
#include "FL/Fl_Tabs.H"
#include "FL/Fl_Text_Buffer.H"
#include "FL/Fl_Text_Display.H"
#include "FL/Fl_Text_Editor.H"
#include "FL/Fl_Tile.H"
#include "FL/Fl_Tiled_Image.H"
#include "FL/Fl_Timer.H"
#include "FL/Fl_Toggle_Button.H"
#include "FL/Fl_Toggle_Light_Button.H"
#include "FL/Fl_Toggle_Round_Button.H"
#include "FL/Fl_Tooltip.H"
#include "FL/Fl_Tree_Item_Array.H"
#include "FL/Fl_Tree_Item.H"
#include "FL/Fl_Tree_Prefs.H"
#include "FL/Fl_Tree.H"
#include "FL/fl_types.h"
#include "FL/fl_utf8.h"
#include "FL/Fl_Valuator.H"
#include "FL/Fl_Value_Input.H"
#include "FL/Fl_Value_Output.H"
#include "FL/Fl_Value_Slider.H"
#include "FL/Fl_Widget.H"
#include "FL/Fl_Window.H"
#include "FL/Fl_Wizard.H"
#include "FL/Fl_XBM_Image.H"
#include "FL/Fl_XPM_Image.H"
#include "FL/Fl.H"

namespace fl
{
    using Global = Fl;
    using Group = Fl_Group;
    using Adjuster = Fl_Adjuster;
    using Bitmap = Fl_Bitmap;
    using BMPImage = Fl_BMP_Image;
    using Box = Fl_Box;
    using Browser = Fl_Browser;
    using Button = Fl_Button;
    using Chart = Fl_Chart;
    using CheckBrowser = Fl_Check_Browser;
    using CheckButton = Fl_Check_Button;
    using Choice = Fl_Choice;
    using Clock = Fl_Clock;
    using ColorChooser = Fl_Color_Chooser;
    using CopySurface = Fl_Copy_Surface;
    using Counter = Fl_Counter;
    using Dial = Fl_Dial;
    using DoubleWindow = Fl_Double_Window;
    using FileBrowser = Fl_File_Browser;
    using FileChooser = Fl_File_Chooser;
    using FileIcon = Fl_File_Icon;
    using FileInput = Fl_File_Input;
    using FillDial = Fl_Fill_Dial;
    using FillSlider = Fl_Fill_Slider;
    using FloatInput = Fl_Float_Input;
    using FormsBitmap = Fl_FormsBitmap;
    using FormsPixmap = Fl_FormsPixmap;
    using Free = Fl_Free;
    using GIFImage = Fl_GIF_Image;
    using GlWindow = Fl_Gl_Window;
    using Group = Fl_Group;
    using HelpDialog = Fl_Help_Dialog;
    using HelpView = Fl_Help_View;
    using HoldBrowser = Fl_Hold_Browser;
    using HorFillSlider = Fl_Hor_Fill_Slider;
    using HorNiceSlider = Fl_Hor_Nice_Slider;
    using HorSlider = Fl_Hor_Slider;
    using HorValueSlider = Fl_Hor_Value_Slider;
    using ImageSurface = Fl_Image_Surface;
    using Image = Fl_Image;
    using InputChoice = Fl_Input_Choice;
    using Input = Fl_Input;
    using IntInput = Fl_Int_Input;
    using JPEGImage = Fl_JPEG_Image;
    using LightButton = Fl_Light_Button;
    using LineDial = Fl_Line_Dial;
    using MenuBar = Fl_Menu_Bar;
    using MenuButton = Fl_Menu_Button;
    using MenuItem = Fl_Menu_Item;
    using MenuWindow = Fl_Menu_Window;
    using Menu = Fl_Menu;
    using MultiBrowser = Fl_Multi_Browser;
    using MultiLabel = Fl_Multi_Label;
    using MultilineInput = Fl_Multiline_Input;
    using MultilineOutput = Fl_Multiline_Output;
    using NativeFile_Chooser = Fl_Native_File_Chooser;
    using NiceSlider = Fl_Nice_Slider;
    using Widget = Fl_Widget;
    using Output = Fl_Output;
    using OverlayWindow = Fl_Overlay_Window;
    using Pack = Fl_Pack;
    using PagedDevice = Fl_Paged_Device;
    using Pixmap = Fl_Pixmap;
    using Plugin = Fl_Plugin;
    using PNGImage = Fl_PNG_Image;
    using PNMImage = Fl_PNM_Image;
    using Positioner = Fl_Positioner;
    using Preferences = Fl_Preferences;
    using Printer = Fl_Printer;
    using Progress = Fl_Progress;
    using RadioButton = Fl_Radio_Button;
    using RadioLight_Button = Fl_Radio_Light_Button;
    using RadioRound_Button = Fl_Radio_Round_Button;
    using RepeatButton = Fl_Repeat_Button;
    using ReturnButton = Fl_Return_Button;
    using RGBImage = Fl_RGB_Image;
    using Roller = Fl_Roller;
    using RoundButton = Fl_Round_Button;
    using RoundClock = Fl_Round_Clock;
    using Scroll = Fl_Scroll;
    using Scrollbar = Fl_Scrollbar;
    using SecretInput = Fl_Secret_Input;
    using SelectBrowser = Fl_Select_Browser;
    using SharedImage = Fl_Shared_Image;
    using SimpleCounter = Fl_Simple_Counter;
    using SingleWindow = Fl_Single_Window;
    using Slider = Fl_Slider;
    using Spinner = Fl_Spinner;
    using SysMenu_ar = Fl_Sys_Menu_Bar;
    using TableRow = Fl_Table_Row;
    using Table = Fl_Table;
    using Tabs = Fl_Tabs;
    using TextBuffer = Fl_Text_Buffer;
    using TextDisplay = Fl_Text_Display;
    using TextEditor = Fl_Text_Editor;
    using Tile = Fl_Tile;
    using TiledImage = Fl_Tiled_Image;
    using Timer = Fl_Timer;
    using ToggleButton = Fl_Toggle_Button;
    using Tooltip = Fl_Tooltip;
    using TreeItem_Array = Fl_Tree_Item_Array;
    using TreeItem = Fl_Tree_Item;
    using TreePrefs = Fl_Tree_Prefs;
    using Tree = Fl_Tree;
    using Valuator = Fl_Valuator;
    using ValueInput = Fl_Value_Input;
    using ValueOutput = Fl_Value_Output;
    using ValueSlider = Fl_Value_Slider;
    using Widget = Fl_Widget;
    using Window = Fl_Window;
    using Wizard = Fl_Wizard;
    using XBMImage = Fl_XBM_Image;
    using XPMImage = Fl_XPM_Image;
}; // namespace fl

#endif