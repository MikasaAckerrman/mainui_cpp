#include <VGUI_ImagePanel.h>
#include <VGUI_Image.h>

namespace vgui
{

ImagePanel::ImagePanel(Image* image) : Panel(0, 0, 64, 64)
{
	_image = image;
}

void ImagePanel::setImage(Image* image)
{
	_image = image;
}

void ImagePanel::paintBackground()
{
	if (_image)
		_image->doPaint(this);
}

}
