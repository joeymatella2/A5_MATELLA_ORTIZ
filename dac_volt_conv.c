/*
 * DAC_volt_conv()
 *
 * MCP4921 control nibble [15:12] = 0x3:
 *   bit 15: /A1A0 = 0  → DAC-A selected
 *   bit 14: BUF   = 0  → unbuffered VREF
 *   bit 13: /GA   = 1  → 1x gain → Vout = (code / 4095) × 3.3 V
 *   bit 12: /SHDN = 1  → output active
 *
 */

#define DAC_VREF        3.3f
#define DAC_FULL_SCALE  4095u
#define MCP4921_CTRL    (0x3u << 12)    /* 0x3000 */

uint16_t DAC_volt_conv(float voltage)
{
    /* Clamp to valid range */
    if (voltage < 0.0f)      voltage = 0.0f;
    if (voltage > DAC_VREF)  voltage = DAC_VREF;

    /* Scale to 12-bit code */
    uint16_t code = (uint16_t)((voltage / DAC_VREF) * (float)DAC_FULL_SCALE + 0.5f);
    code &= 0x0FFFu;                    /* mask to 12 bits (defensive) */

    /* Prepend control nibble */
    return (uint16_t)(MCP4921_CTRL | code);
}
