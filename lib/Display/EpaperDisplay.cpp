#include "EpaperDisplay.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

EpaperDisplay::EpaperDisplay(PROTOCOL::Spi *spi, gpio_num_t pin_dc, gpio_num_t pin_rst, gpio_num_t pin_busy)
    : spi(spi), pin_dc(pin_dc), pin_rst(pin_rst), pin_busy(pin_busy)
{
    gpio_pin_dc = Perifericos::Gpio(pin_dc, GPIO_MODE_OUTPUT, GPIO_PULLUP_ONLY);
    gpio_pin_rst = Perifericos::Gpio(pin_rst, GPIO_MODE_OUTPUT, GPIO_PULLUP_ONLY);
    gpio_pin_busy = Perifericos::Gpio(pin_busy, GPIO_MODE_INPUT, GPIO_PULLDOWN_ONLY);
    this->_hibernating = 1;
}

uint8_t EpaperDisplay::init()
{
    vTaskDelay(pdMS_TO_TICKS(20)); // Delay após o reset hardware
    if (this->_hibernating)
    {
        reset();
        ESP_LOGW("EPD-INIT", "reset");
        waitForIdle(); // Espera pelo pino busy
    }

    // HW Reset

    epd_write_reg(0x12);          // Software reset
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    waitForIdle();                // Espera pelo pino busy

    epd_write_reg(0x01);          // Driver output control
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x27);
    epd_write_data(0x01);
    epd_write_data(0x01);
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_write_reg(0x11);          // data entry mode
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x01);
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_write_reg(0x44);          // set Ram-X address start/end position
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x00);
    epd_write_data(0x0F);         // 0x0F-->(15+1)*8=128
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_write_reg(0x45);          // set Ram-Y address start/end position
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x27);         // 0x127-->(295+1)=296
    epd_write_data(0x01);
    epd_write_data(0x00);
    epd_write_data(0x00);
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_write_reg(0x3C);          // BorderWavefrom
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x05);
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_write_reg(0x18);          // Read built-in temperature sensor
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay
    epd_write_data(0x80);
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay

    epd_setpos(0, 0);

    if (epd_power_on())
        return 1;

    ESP_LOGW("EPD-INIT", "OK");
    return 0;
}

uint8_t EpaperDisplay::epd_init_partial(void)
{
    static const unsigned char lut_partial[] =
        {
            0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x40, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
            0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0};
    if (init())
        return 1;

    epd_write_reg(0x32);
    for (int j = 0; j < sizeof(lut_partial); j++)
    {
        epd_write_data(lut_partial[j]);
    }

    return 0;
}
void EpaperDisplay::epd_setpos(uint16_t x, uint16_t y)
{
    uint8_t _x;
    uint16_t _y;
    _x = x / 8;
    _y = 295 - y;

    epd_write_reg(0x4E); // set RAM x address count to 0;
    epd_write_data(_x);
    epd_write_reg(0x4F); // set RAM y address count to 0x127;
    epd_write_data(_y & 0xff);
    epd_write_data(_y >> 8 & 0x01);
}

void EpaperDisplay::reset()
{
    this->_hibernating = 0;
    gpio_pin_rst.set_level(false);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_pin_rst.set_level(true);
}

void EpaperDisplay::epd_write_reg(uint8_t command)
{

    gpio_pin_dc.set_level(false);     // DC low for command
    spi->transferByte(0, command, 0); // Sending command
    gpio_pin_dc.set_level(false);     // DC high for data
}
void EpaperDisplay::epd_write_data_(const uint8_t *data, size_t length)
{

    gpio_pin_dc.set_level(true); // DC high for data
    for (size_t i = 0; i < length; ++i)
    {
        spi->transferByte(0, data[i], 0); // Assume '0' for reg_addr and '1' for command if needed
    }
    gpio_pin_dc.set_level(true); // DC high for data
}
void EpaperDisplay::epd_write_data(uint8_t value)
{
    gpio_pin_dc.set_level(true);    // DC low for command
    spi->transferByte(0, value, 0); // Sending command
    gpio_pin_dc.set_level(true);    // DC high for data
}
uint8_t EpaperDisplay::epd_power_on(void)
{
    epd_write_reg(0x22); // Display Update Control
    epd_write_data(0xf8);
    epd_write_reg(0x20); // Activate Display Update Sequence

    return epd_wait_busy();
}
void EpaperDisplay::epd_update_partial(void)
{
    epd_write_reg(0x22); // Display Update Control
    epd_write_data(0xCC);
    epd_write_reg(0x20); // Activate Display Update Sequence

    epd_wait_busy();
}
void EpaperDisplay::epd_update(void)
{
    epd_write_reg(0x22); // Display Update Control
    epd_write_data(0xF7);
    epd_write_reg(0x20); // Activate Display Update Sequence

    epd_wait_busy();
}
uint8_t EpaperDisplay::epd_power_off(void)
{
    epd_write_reg(0x22); // Display Update Control
    epd_write_data(0x83);
    epd_write_reg(0x20); // Activate Display Update Sequence

    return epd_wait_busy();
}
void EpaperDisplay::epd_enter_deepsleepmode(uint8_t mode)
{
    epd_power_off();
    epd_write_reg(0x10);
    epd_write_data(mode);
    _hibernating = 1;
}
void EpaperDisplay::epd_init_internalTempSensor(void)
{
    epd_write_reg(0x18);
    epd_write_data(0x80);

    epd_write_reg(0x1A);
    epd_write_data(0x7F);
    epd_write_data(0xF0);
}

void EpaperDisplay::sleep(uint8_t mode)
{
    epd_power_off();
    epd_write_reg(0x10);
    epd_write_data(mode);
    this->_hibernating = 1;
}

void EpaperDisplay::waitForIdle()
{
    while (gpio_pin_busy.get_level() == 1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI("EP_IDLE", "IDLE");
}
uint8_t EpaperDisplay::epd_wait_busy()
{
    uint32_t timeout = 0;
    while (gpio_pin_busy.get_level() == 1)
    {
        timeout++;
        if (timeout > 40000)
        {
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return 0;
}

void EpaperDisplay::epd_display(uint8_t *Image1, uint8_t *Image2)
{
    uint32_t Width, Height, i, j;
    uint32_t k = 0;
    Width = EPD_H;
    Height = EPD_W_BUFF_SIZE;

    epd_setpos(0, 0);

    epd_write_reg(0x24);
    epd_write_data_(Image1, Width * Height);

    epd_setpos(0, 0);

    epd_write_reg(0x26);
    k = 0;
    for (j = 0; j < Height; j++)
    {
        for (i = 0; i < Width; i++)
        {
            epd_write_data(~Image2[k]);
            k++;
        }
    }

    epd_update();
}

void EpaperDisplay::epd_displayBW(uint8_t *Image)
{
    uint32_t Width, Height;

    Width = EPD_H;
    Height = EPD_W_BUFF_SIZE;

    epd_setpos(0, 0);
    epd_write_reg(0x26);
    epd_write_data_(Image, Width * Height);

    epd_setpos(0, 0);
    epd_write_reg(0x24);
    epd_write_data_(Image, Width * Height);

    epd_update();
}
void EpaperDisplay::epd_displayBW_partial(uint8_t *Image)
{
    uint32_t Width, Height;

    Width = EPD_H;
    Height = EPD_W_BUFF_SIZE;

    epd_setpos(0, 0);
    epd_write_reg(0x24);
    epd_write_data_(Image, Width * Height);

    epd_update_partial();

    epd_setpos(0, 0);
    epd_write_reg(0x26);
    epd_write_data_(Image, Width * Height);
}
void EpaperDisplay::epd_displayRED(uint8_t *Image)
{
    uint32_t Width, Height;

    Width = EPD_H;
    Height = EPD_W_BUFF_SIZE;

    epd_setpos(0, 0);

    epd_write_reg(0x26);
    epd_write_data_(Image, Width * Height);

    epd_update();
}
void EpaperDisplay::epd_paint_newimage(uint8_t *image, uint16_t Width, uint16_t Height, uint16_t Rotate, uint16_t Color)
{

    EPD_Paint.Image = 0x00;
    EPD_Paint.Image = image;

    EPD_Paint.WidthMemory = Width;
    EPD_Paint.HeightMemory = Height;
    EPD_Paint.Color = Color;
    EPD_Paint.WidthByte = (Width % 8 == 0) ? (Width / 8) : (Width / 8 + 1);
    EPD_Paint.HeightByte = Height;
    EPD_Paint.Rotate = Rotate;
    if (Rotate == EPD_ROTATE_0 || Rotate == EPD_ROTATE_180)
    {

        EPD_Paint.Width = Height;
        EPD_Paint.Height = Width;
    }
    else
    {
        EPD_Paint.Width = Width;
        EPD_Paint.Height = Height;
    }
}

void EpaperDisplay::epd_paint_setpixel(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    uint16_t X, Y;
    uint32_t Addr;
    uint8_t Rdata;
    switch (EPD_Paint.Rotate)
    {
    case 0:

        X = EPD_Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 90:
        X = EPD_Paint.WidthMemory - Xpoint - 1;
        Y = EPD_Paint.HeightMemory - Ypoint - 1;
        break;
    case 180:
        X = Ypoint;
        Y = EPD_Paint.HeightMemory - Xpoint - 1;
        break;
    case 270:
        X = Xpoint;
        Y = Ypoint;
        break;
    default:
        return;
    }
    Addr = X / 8 + Y * EPD_Paint.WidthByte;
    Rdata = EPD_Paint.Image[Addr];
    if (Color == EPD_COLOR_BLACK)
    {
        EPD_Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
    }
    else
        EPD_Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
}
void EpaperDisplay::epd_paint_clear(uint16_t color)
{

    uint16_t X, Y;
    uint32_t Addr;

    for (Y = 0; Y < EPD_Paint.HeightByte; Y++)
    {
        for (X = 0; X < EPD_Paint.WidthByte; X++)
        { // 8 pixel =  1 byte
            Addr = X + Y * EPD_Paint.WidthByte;
            EPD_Paint.Image[Addr] = color;
        }
    }
}
void EpaperDisplay::epd_paint_selectimage(uint8_t *image)
{
    EPD_Paint.Image = image;
}

void EpaperDisplay::epd_paint_drawPoint(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    epd_paint_setpixel(Xpoint - 1, Ypoint - 1, Color);
}
void EpaperDisplay::epd_paint_drawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color)
{
    uint16_t Xpoint, Ypoint;
    int32_t dx, dy;
    int32_t XAddway, YAddway;
    int32_t Esp;
    char Dotted_Len;
    Xpoint = Xstart;
    Ypoint = Ystart;
    dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    XAddway = Xstart < Xend ? 1 : -1;
    YAddway = Ystart < Yend ? 1 : -1;

    Esp = dx + dy;
    Dotted_Len = 0;

    for (;;)
    {
        Dotted_Len++;
        epd_paint_drawPoint(Xpoint, Ypoint, Color);
        if (2 * Esp >= dy)
        {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx)
        {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}
void EpaperDisplay::epd_paint_drawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color, uint8_t mode)
{
    uint16_t i;
    if (mode)
    {
        for (i = Ystart; i < Yend; i++)
        {
            epd_paint_drawLine(Xstart, i, Xend, i, Color);
        }
    }
    else
    {
        epd_paint_drawLine(Xstart, Ystart, Xend, Ystart, Color);
        epd_paint_drawLine(Xstart, Ystart, Xstart, Yend, Color);
        epd_paint_drawLine(Xend, Yend, Xend, Ystart, Color);
        epd_paint_drawLine(Xend, Yend, Xstart, Yend, Color);
    }
}
void EpaperDisplay::epd_paint_drawCircle(uint16_t X_Center, uint16_t Y_Center, uint16_t Radius, uint16_t Color, uint8_t mode)
{
    int Esp;
    uint16_t XCurrent, YCurrent, sCountY;
    XCurrent = 0;
    YCurrent = Radius;
    Esp = 3 - (Radius << 1);
    if (mode)
    {
        while (XCurrent <= YCurrent)
        { // Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++)
            {
                epd_paint_drawPoint(X_Center + XCurrent, Y_Center + sCountY, Color); // 1
                epd_paint_drawPoint(X_Center - XCurrent, Y_Center + sCountY, Color); // 2
                epd_paint_drawPoint(X_Center - sCountY, Y_Center + XCurrent, Color); // 3
                epd_paint_drawPoint(X_Center - sCountY, Y_Center - XCurrent, Color); // 4
                epd_paint_drawPoint(X_Center - XCurrent, Y_Center - sCountY, Color); // 5
                epd_paint_drawPoint(X_Center + XCurrent, Y_Center - sCountY, Color); // 6
                epd_paint_drawPoint(X_Center + sCountY, Y_Center - XCurrent, Color); // 7
                epd_paint_drawPoint(X_Center + sCountY, Y_Center + XCurrent, Color);
            }
            if (Esp < 0)
            {
                Esp += 4 * XCurrent + 6;
            }
            else
            {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                YCurrent--;
            }
            XCurrent++;
        }
    }
    else
    { // Draw a hollow circle
        while (XCurrent <= YCurrent)
        {
            epd_paint_drawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color); // 1
            epd_paint_drawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color); // 2
            epd_paint_drawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color); // 3
            epd_paint_drawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color); // 4
            epd_paint_drawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color); // 5
            epd_paint_drawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color); // 6
            epd_paint_drawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color); // 7
            epd_paint_drawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color); // 0
            if (Esp < 0)
            {
                Esp += 4 * XCurrent + 6;
            }
            else
            {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                YCurrent--;
            }
            XCurrent++;
        }
    }
}

void EpaperDisplay::epd_paint_showChar(uint16_t x, uint16_t y, uint16_t chr, uint16_t size1, uint16_t color)
{
    uint16_t i, m, temp, size2, chr1;
    uint16_t x0, y0;
    x += 1, y += 1, x0 = x, y0 = y;
    if (x - size1 > EPD_H)
        return;
    if (size1 == 8)
        size2 = 6;
    else
        size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);
    chr1 = chr - ' ';
    for (i = 0; i < size2; i++)
    {
        if (size1 == 8)
        {
            temp = asc2_0806[chr1][i];
        } // 0806
        else if (size1 == 12)
        {
            temp = asc2_1206[chr1][i];
        } // 1206
        else if (size1 == 16)
        {
            temp = asc2_1608[chr1][i];
        } // 1608
        else if (size1 == 24)
        {
            temp = asc2_2412[chr1][i];
        } // 2412
        else
            return;
        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                epd_paint_drawPoint(x, y, color);
            else
                epd_paint_drawPoint(x, y, !color);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2))
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}
void EpaperDisplay::epd_paint_showString(uint16_t x, uint16_t y, uint8_t *chr, uint16_t size1, uint16_t color)
{
    while (*chr != '\0')
    {
        epd_paint_showChar(x, y, *chr, size1, color);
        chr++;
        if (size1 == 8)
        {
            x += 6;
            if (x > EPD_H - 6)
                break;
        }
        else
        {
            x += size1 / 2;
            if (x > EPD_H - size1 / 2)
                break;
        }
    }
}
uint32_t EpaperDisplay::_Pow(uint16_t m, uint16_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result;
}
void EpaperDisplay::epd_paint_showNum(uint16_t x, uint16_t y, uint32_t num, uint16_t len, uint16_t size1, uint16_t color)
{
    uint8_t t, temp, m = 0;
    if (size1 == 8)
        m = 2;
    for (t = 0; t < len; t++)
    {
        temp = (num / _Pow(10, len - t - 1)) % 10;
        if (temp == 0)
        {
            epd_paint_showChar(x + (size1 / 2 + m) * t, y, '0', size1, color);
        }
        else
        {
            epd_paint_showChar(x + (size1 / 2 + m) * t, y, temp + '0', size1, color);
        }
    }
}
void EpaperDisplay::epd_paint_showChinese(uint16_t x, uint16_t y, uint16_t num, uint16_t size1, uint16_t color)
{
    uint16_t m, temp;
    uint16_t x0, y0;
    uint16_t i, size3 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * size1;
    x += 1, y += 1, x0 = x, y0 = y;
    for (i = 0; i < size3; i++)
    {
        if (size1 == 16)
        {
            temp = Hzk1[num][i];
        } // 16*16
        else if (size1 == 24)
        {
            temp = Hzk2[num][i];
        } // 24*24
        else if (size1 == 32)
        {
            temp = Hzk3[num][i];
        } // 32*32
        else if (size1 == 64)
        {
            temp = Hzk4[num][i];
        } // 64*64
        else
            return;
        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                epd_paint_drawPoint(x, y, color);
            else
                epd_paint_drawPoint(x, y, !color);
            temp >>= 1;
            y++;
        }
        x++;
        if ((x - x0) == size1)
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}
void EpaperDisplay::epd_paint_showPicture(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t BMP[], uint16_t Color)
{
    uint16_t j = 0;
    uint16_t i, n = 0, temp = 0, m = 0;
    uint16_t x0 = 0, y0 = 0;
    x += 1, y += 1, x0 = x, y0 = y;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
    for (n = 0; n < sizey; n++)
    {
        for (i = 0; i < sizex; i++)
        {
            temp = BMP[j];
            j++;
            for (m = 0; m < 8; m++)
            {
                if (temp & 0x01)
                    epd_paint_drawPoint(x, y, !Color);
                else
                    epd_paint_drawPoint(x, y, Color);
                temp >>= 1;
                y++;
            }
            x++;
            if ((x - x0) == sizex)
            {
                x = x0;
                y0 = y0 + 8;
            }
            y = y0;
        }
    }
}
