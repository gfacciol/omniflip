import wx

MAIN_WINDOW_DEFAULT_SIZE = (300,200)

class Frame(wx.Frame):
    
    def __init__(self, parent, id, title):
        style=wx.DEFAULT_FRAME_STYLE ^ (wx.RESIZE_BORDER) # XOR to remove the resizeable border        
        wx.Frame.__init__(self, parent, id, title=title, size=MAIN_WINDOW_DEFAULT_SIZE, style=style)
        self.Center() # open in the centre of the screen
        self.panel = wx.Panel(self)
        self.panel.SetBackgroundColour('White') # make the background of the window white

class App(wx.App):
    
    def OnInit(self):
        self.frame = Frame(parent=None, id=-1, title='Image Viewer')
        self.frame.Show()
        self.SetTopWindow(self.frame)
        return True
    
if __name__ == "__main__":       
    # make an App object, set stdout to the console so we can see errors
    app = App()
        
    app.MainLoop()
