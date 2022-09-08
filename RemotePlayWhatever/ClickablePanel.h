#ifndef CLICKABLEPANEL_H
#define CLICKABLEPANEL_H
#include <wx/wx.h>

wxDECLARE_EVENT(BTN_PANEL_LEFT_CLICK, wxCommandEvent);

class ClickablePanel: public wxPanel
{
public:
    ClickablePanel(wxWindow* parent, wxWindowID winID);
    ~ClickablePanel();

protected:
    void RBindBaseEvents();

    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);

private:
    void RSetForegroundColour(const wxColour &colour);
    void RSetBackgroundColour(const wxColour &colour);
};

class BtnPanel: public ClickablePanel
{
public:
    BtnPanel(wxWindow* parent, wxWindowID winID, wxString label);
    ~BtnPanel();

    void SetLabel(wxString& label);

private:
    wxStaticText* m_label;
};

#endif // CLICKABLEPANEL_H
