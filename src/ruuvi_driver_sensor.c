#include "ruuvi_driver_error.h"
#include "ruuvi_driver_sensor.h"
#include <stddef.h>
#include <string.h>

static const char m_init_name[] = "NOTINIT";

rd_status_t rd_sensor_configuration_set (const rd_sensor_t *
        sensor, rd_sensor_configuration_t * config)
{
    rd_status_t err_code = RD_SUCCESS;

    if (NULL == sensor || NULL == config) { return RD_ERROR_NULL; }

    if (NULL == sensor->samplerate_set) { return RD_ERROR_INVALID_STATE; }

    uint8_t sleep = RD_SENSOR_CFG_SLEEP;
    err_code |= sensor->mode_set (&sleep);
    err_code |= sensor->samplerate_set (& (config->samplerate));
    err_code |= sensor->resolution_set (& (config->resolution));
    err_code |= sensor->scale_set (& (config->scale));
    err_code |= sensor->dsp_set (& (config->dsp_function), & (config->dsp_parameter));
    err_code |= sensor->mode_set (& (config->mode));
    return err_code;
}

rd_status_t rd_sensor_configuration_get (const rd_sensor_t *
        sensor, rd_sensor_configuration_t * config)
{
    rd_status_t err_code = RD_SUCCESS;

    if (NULL == sensor || NULL == config) { return RD_ERROR_NULL; }

    if (NULL == sensor->samplerate_set) { return RD_ERROR_INVALID_STATE; }

    err_code |= sensor->samplerate_get (& (config->samplerate));
    err_code |= sensor->resolution_get (& (config->resolution));
    err_code |= sensor->scale_get (& (config->scale));
    err_code |= sensor->dsp_get (& (config->dsp_function), & (config->dsp_parameter));
    err_code |= sensor->mode_get (& (config->mode));
    return err_code;
}

static rd_sensor_timestamp_fp millis = NULL;

rd_status_t rd_sensor_timestamp_function_set (
    rd_sensor_timestamp_fp timestamp_fp)
{
    millis = timestamp_fp;
    return RD_SUCCESS;
}

// Calls the timestamp function and returns it's value. returns 0 if timestamp function is NULL
uint64_t rd_sensor_timestamp_get (void)
{
    if (NULL == millis)
    {
        return 0;
    }

    return millis();
}

bool rd_sensor_is_init (const rd_sensor_t * const sensor)
{
    return (strcmp (sensor->name, m_init_name));
}

static rd_status_t rd_fifo_enable_ni (const bool enable)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_fifo_interrupt_enable_ni (const bool enable)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_fifo_read_ni (size_t * num_elements, rd_sensor_data_t * data)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_data_get_ni (rd_sensor_data_t * const data)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_init_ni (rd_sensor_t * const
                               p_sensor, const rd_bus_t bus, const uint8_t handle)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_setup_ni (uint8_t * const value)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_level_interrupt_use_ni (const bool enable,
        float * limit_g)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_dsp_ni (uint8_t * const dsp, uint8_t * const parameter)
{
    return RD_ERROR_NOT_INITIALIZED;
}

static rd_status_t rd_sensor_configuration_ni (const rd_sensor_t *
        sensor, rd_sensor_configuration_t * config)
{
    return RD_ERROR_NOT_INITIALIZED;
}

void rd_sensor_initialize (rd_sensor_t * const p_sensor)
{
    p_sensor->name                  = m_init_name;
    p_sensor->configuration_get     = rd_sensor_configuration_ni;
    p_sensor->configuration_set     = rd_sensor_configuration_ni;
    p_sensor->data_get              = rd_data_get_ni;
    p_sensor->dsp_get               = rd_dsp_ni;
    p_sensor->dsp_set               = rd_dsp_ni;
    p_sensor->fifo_enable           = rd_fifo_enable_ni;
    p_sensor->fifo_interrupt_enable = rd_fifo_interrupt_enable_ni;
    p_sensor->fifo_read             = rd_fifo_read_ni;
    p_sensor->init                  = rd_init_ni;
    p_sensor->uninit                = rd_init_ni;
    p_sensor->level_interrupt_set   = rd_level_interrupt_use_ni;
    p_sensor->mode_get              = rd_setup_ni;
    p_sensor->mode_set              = rd_setup_ni;
    p_sensor->resolution_get        = rd_setup_ni;
    p_sensor->resolution_set        = rd_setup_ni;
    p_sensor->samplerate_get        = rd_setup_ni;
    p_sensor->samplerate_set        = rd_setup_ni;
    p_sensor->scale_get             = rd_setup_ni;
    p_sensor->scale_set             = rd_setup_ni;
    memset (& (p_sensor->provides), 0, sizeof (p_sensor->provides));
}

void rd_sensor_uninitialize (rd_sensor_t * const p_sensor)
{
    // Reset sensor to initial values.
    rd_sensor_initialize (p_sensor);
}

static inline uint8_t get_index_of_field (const rd_sensor_data_t * const target,
        const rd_sensor_data_fields_t field)
{
    // Null bits higher than target
    uint32_t mask = (1 << (32 - __builtin_clz (field.bitfield))) - 1;
    // Count set bits in nulled bitfield to find index.
    return __builtin_popcount (target->fields.bitfield & mask) - 1;
}

float rd_sensor_data_parse (const rd_sensor_data_t * const provided,
                            const rd_sensor_data_fields_t requested)
{
    // If there isn't valid requested data, return value "invalid".
    if (! (provided->valid.bitfield & requested.bitfield)) { return RD_FLOAT_INVALID; }

    // If trying to get more than one field, return value "invalid".
    if (1 != __builtin_popcount (requested.bitfield)) { return RD_FLOAT_INVALID; }

    // Return requested value
    return provided->data[get_index_of_field (provided, requested)];
}

void rd_sensor_data_set (rd_sensor_data_t * const target,
                         const rd_sensor_data_fields_t field,
                         const float value)
{
    // If there isn't valid requested data, return
    if (! (target->fields.bitfield & field.bitfield)) { return; }

    // If trying to set more than one field, return.
    if (1 != __builtin_popcount (field.bitfield)) { return; }

    // Set value to appropriate index
    target->data[get_index_of_field (target, field)] = value;
    // Mark data as valid
    target->valid.bitfield |= field.bitfield;
}

void rd_sensor_data_populate (rd_sensor_data_t * const target,
                              const rd_sensor_data_t * const provided,
                              const rd_sensor_data_fields_t requested)
{
    if (NULL == target || NULL == provided) { return; }

    // Compare provided data to requested data.
    rd_sensor_data_fields_t available = {.bitfield = (provided->valid).bitfield & requested.bitfield};

    // We have the available, requested fields. Fill the target struct with those
    while (available.bitfield)
    {
        // read rightmost field
        rd_sensor_data_fields_t next = {.bitfield = (1 << __builtin_ctz (available.bitfield)) };
        float value = rd_sensor_data_parse (provided, next);
        rd_sensor_data_set (target, next, value);
        available.bitfield &= (available.bitfield - 1); // set rightmost bit of available to 0
    }
}

inline uint8_t rd_sensor_data_fieldcount (const rd_sensor_data_t * const target)
{
    return __builtin_popcount (target->fields.bitfield);
}