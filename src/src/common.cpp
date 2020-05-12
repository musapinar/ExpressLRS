#include "common.h"

void commonConfig()
{ //settings common to both master and slave
}

extern SX127xDriver Radio;

// TODO: Validate values for RFmodeCycleAddtionalTime and RFmodeCycleInterval for rates lower than 50HZ

#define RATE_MAX 3
const expresslrs_mod_settings_s ExpressLRS_AirRateConfig[RATE_MAX] = {
    {BW_500_00_KHZ, SF_6, CR_4_5, -112, 5000, 200, TLM_RATIO_1_64, 4, 8, RATE_200HZ, 1000, 1500},
    {BW_500_00_KHZ, SF_7, CR_4_7, -117, 10000, 100, TLM_RATIO_1_32, 4, 8, RATE_100HZ, 2000, 2000},
    {BW_500_00_KHZ, SF_8, CR_4_7, -120, 20000, 50, TLM_RATIO_1_16, 2, 10, RATE_50HZ, 6000, 2500},
    //{BW_250_00_KHZ, SF_8, CR_4_7, -123, 40000, 25, TLM_RATIO_NO_TLM, 2, 8, RATE_25HZ, 6000, 2500}, // not using thse slower rates for now
    //{BW_250_00_KHZ, SF_11, CR_4_5, -131, 250000, 4, TLM_RATIO_NO_TLM, 2, 8, RATE_4HZ, 6000, 2500},
};

const expresslrs_mod_settings_s * get_elrs_airRateConfig(expresslrs_RFrates_e rate)
{
    // Protect against out of bounds rate
    if (rate < 0) {
        // Set to first entry in the array (200HZ)
        return &ExpressLRS_AirRateConfig[0];
    }
    else if (rate > MaxRFrate) {
        // Set to last usable entry in the array (currently 50HZ)
        return &ExpressLRS_AirRateConfig[MaxRFrate];
    }
        
    return &ExpressLRS_AirRateConfig[rate];
}

//const expresslrs_mod_settings_s * ExpressLRS_nextAirRate;
const expresslrs_mod_settings_s * ExpressLRS_currAirRate;
const expresslrs_mod_settings_s * ExpressLRS_prevAirRate;

int8_t ExpressLRS_currPower = 0;
int8_t ExpressLRS_prevPower = 0;

connectionState_e connectionState = disconnected;
connectionState_e connectionStatePrev = disconnected;

uint8_t UID[6] = {0}; // <- TODO set to bind UID so a new Rx defaults to binding
#ifndef MY_UID
    #ifdef PLATFORM_ESP32
        esp_err_t WiFiErr = esp_read_mac(UID, ESP_MAC_WIFI_STA);
    #endif
    #ifdef TARGET_R9M_TX
        UID[6] = {HAL_GetUIDw0(), HAL_GetUIDw0() >> 8, HAL_GetUIDw1(), HAL_GetUIDw1() >> 8, HAL_GetUIDw2(), HAL_GetUIDw2() >> 8};
    #endif
#else
    UID[6] = {MY_UID};
#endif

uint8_t CRCCaesarCipher = UID[4];
uint8_t DeviceAddr = UID[5] & 0b111111; // temporarily based on mac until listen before assigning method merged

#define RSSI_FLOOR_NUM_READS 5 // number of times to sweep the noise foor to get avg. RSSI reading
#define MEDIAN_SIZE 20

int16_t MeasureNoiseFloor()
{
    int NUM_READS = RSSI_FLOOR_NUM_READS * NR_FHSS_ENTRIES;
    float returnval = 0;

    for (uint32_t freq = 0; freq < NR_FHSS_ENTRIES; freq++)
    {
        FHSSsetCurrIndex(freq);
        Radio.SetMode(SX127X_CAD);

        for (int i = 0; i < RSSI_FLOOR_NUM_READS; i++)
        {
            returnval = returnval + Radio.GetCurrRSSI();
            delay(5);
        }
    }
    returnval = returnval / NUM_READS;
    return (returnval);
}

uint8_t TLMratioEnumToValue(expresslrs_tlm_ratio_e enumval)
{
    switch (enumval)
    {
    case TLM_RATIO_NO_TLM:
        return 0;
        break;
    case TLM_RATIO_1_2:
        return 2;
        break;
    case TLM_RATIO_1_4:
        return 4;
        break;
    case TLM_RATIO_1_8:
        return 8;
        break;
    case TLM_RATIO_1_16:
        return 16;
        break;
    case TLM_RATIO_1_32:
        return 32;
        break;
    case TLM_RATIO_1_64:
        return 64;
        break;
    case TLM_RATIO_1_128:
        return 128;
        break;
    default:
        return 0xFF;
    }
}
