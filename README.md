# Input Buffering plug-in for Unreal Engine 4
##Features
* A new player controller class with an input buffer that allows developers to set up input events and store them in the buffer for future examination.
* A new asset type of Input Command that consists of sequences of input events and can represent typical input commands such as Quarter-Circle-Forward Punch commonly found in fighting games
* Ability to tell whether given Input Commands match the contents of input buffer.

##Documentation
To get a quick start, please follow [this link](https://ue4inputbuffer.wordpress.com/).

##Installation
To add source code of the plug-in to Unreal Engine 4 C++ project, follow the steps below:

* Download a zip of the source files via GitHub.
* Create a "Plugins" folder in your project root directory and then create an "InputBuffer" subfolder in the "Plugins" folder.
* Decompress the downloaded zip to the "InputBuffer" subfolder.
* In Windows file explorer, find your Unreal project file "XXX.uproject" under its root directory and right click it to bring up a content menu, then click "Generate Visual Studio project files".
* Open the Visual Studio solution file of your project, you should be able to see the source files of the plug-in added.
