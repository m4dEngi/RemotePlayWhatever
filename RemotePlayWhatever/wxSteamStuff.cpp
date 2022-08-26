#include "wxSteamStuff.h"

wxBitmap SteamImageToBitmap(int imageID)
{
    uint32 imgHeight, imgWidth;
    wxImage wImage;
    if(GClientContext()->SteamUtils()->GetImageSize(imageID, &imgWidth, &imgHeight))
    {
        size_t bufSize = imgHeight * imgWidth;
        uint32_t* buf = new uint32_t[bufSize];

        if(GClientContext()->SteamUtils()->GetImageRGBA(imageID, (uint8_t*)buf, bufSize * 4))
        {
            size_t imgSize = bufSize * 3;
            uint8_t* imBuf = (uint8_t*)malloc(imgSize);
            for(size_t i = 0; i < bufSize; ++i)
            {
                int offset = i * 3;
                imBuf[offset]     = ((uint8_t*)&buf[i])[0];
                imBuf[offset + 1] = ((uint8_t*)&buf[i])[1];
                imBuf[offset + 2] = ((uint8_t*)&buf[i])[2];
            }
            wImage.SetData(imBuf, imgWidth, imgHeight);
        }

        delete[] buf;
    }

    return wxBitmap(wImage);
}

wxSteamID::wxSteamID(CSteamID steamID):
    m_steamID(steamID)
{
}

CSteamID wxSteamID::GetSteamID()
{
    return m_steamID;
}
