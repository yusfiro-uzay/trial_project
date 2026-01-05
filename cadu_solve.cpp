/**
 * =============================================================================
 * OQPSK/BPSK File Demodulator
 * =============================================================================
 *
 * Description:
 *   Software demodulator for OQPSK and BPSK modulated IQ baseband files.
 *   Implements Costas loop for carrier recovery and Mueller & Müller timing
 *   recovery. Includes optional blind processing chain and Reed-Solomon
 *   decoding for CCSDS-compatible frames.
 *
 * Features:
 *   - OQPSK and BPSK demodulation
 *   - 16-bit and 32-bit IQ input support
 *   - Configurable RRC matched filtering
 *   - Optional low-pass pre-filtering
 *   - Costas loop carrier recovery
 *   - Mueller & Müller timing recovery
 *   - Auto-tuning of loop parameters (optional)
 *   - Blind processing: Viterbi decoding, NRZ-M, PSR descrambling
 *   - CCSDS frame sync and Reed-Solomon decoding
 *   - UDP bit streaming (optional)
 *
 * Compatibility:
 *   - MSVC compatible (no C99 complex.h dependency)
 *   - Cross-platform (Windows/Linux)
 *
 * Usage:
 *   demod [options]
 *   -i FILE         Input IQ file
 *   -d NUM          Decimation factor
 *   --sps NUM       Samples per symbol
 *   --bpsk          BPSK demodulation mode
 *   --oqpsk         OQPSK demodulation mode (default)
 *   --iq16          16-bit IQ input format (default)
 *   --iq32          32-bit IQ input format
 *   --help          Show help message
 *
 * =============================================================================
 */

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

/* =============================================================================
 * STANDARD LIBRARY INCLUDES
 * =============================================================================
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =============================================================================
 * PLATFORM-SPECIFIC INCLUDES
 * =============================================================================
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/* =============================================================================
 * PROJECT-SPECIFIC INCLUDES
 * =============================================================================
 */
#include "_rs_decode.c"
#include "init_rs.c"

/* =============================================================================
 * MATHEMATICAL CONSTANTS
 * =============================================================================
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* *****************************************************************************
 *
 *                         CONFIGURATION SECTION
 *
 * *****************************************************************************/

/* =============================================================================
 * MODULATION MODE SELECTION
 * -----------------------------------------------------------------------------
 * Select the modulation scheme for demodulation.
 *   MOD_OQPSK  - Offset QPSK (2 bits/symbol, I/Q offset by half symbol)
 *   MOD_BPSK   - Binary PSK (1 bit/symbol)
 * =============================================================================
 */
#define MOD_OQPSK 0
#define MOD_BPSK 1
#define MODULATION MOD_OQPSK /* Active modulation mode */

/* =============================================================================
 * INPUT FORMAT SELECTION
 * -----------------------------------------------------------------------------
 * Select the input file format for IQ samples.
 *   FMT_IQ16  - 16-bit signed integers (int16_t I, int16_t Q pairs)
 *   FMT_IQ32  - 32-bit signed integers (int32_t I, int32_t Q pairs)
 * =============================================================================
 */
#define FMT_IQ16 0
#define FMT_IQ32 1
#define INPUT_FORMAT FMT_IQ16 /* Active input format */

/* =============================================================================
 * PROCESSING CHAIN TOGGLES
 * -----------------------------------------------------------------------------
 * Enable/disable individual processing stages.
 * Set to 1 to enable, 0 to disable.
 * =============================================================================
 */
#define ENABLE_CONVOLUTION 0 /* Viterbi convolutional decoding         */
#define ENABLE_NRZM 1        /* NRZ-M differential decoding            */
#define ENABLE_PSR 1         /* CCSDS pseudo-random descrambling       */
#define ENABLE_AUTO_TUNE 0/* Automatic loop parameter optimization  */
#define ENABLE_LOWPASS 1     /* Low-pass pre-filter before demod       */
#define ENABLE_UDP_SENDER 0  /* Stream demodulated bits via UDP        */

/* =============================================================================
 * LOW-PASS FILTER CONFIGURATION
 * -----------------------------------------------------------------------------
 * Pre-demodulation low-pass filter settings (when ENABLE_LOWPASS=1)
 * =============================================================================
 */
#define LOWPASS_CUTOFF_NORM 7.5f /* Normalized cutoff frequency        */
#define LOWPASS_NTAPS 101        /* Number of FIR filter taps          */

/* =============================================================================
 * UDP STREAMING CONFIGURATION
 * -----------------------------------------------------------------------------
 * Network settings for bit streaming (when ENABLE_UDP_SENDER=1)
 * =============================================================================
 */
#define UDP_HOST "127.0.0.1" /* Destination IP address        */
#define UDP_PORT 4040        /* Destination UDP port          */
#define UDP_CHUNK_SIZE 40928 /* Bytes per UDP packet          */
#define UDP_DELAY_MS 100     /* Inter-packet delay (ms)       */

/* =============================================================================
 * CCSDS FRAME PARAMETERS
 * -----------------------------------------------------------------------------
 * Frame structure for CCSDS telemetry decoding
 * =============================================================================
 */
#define FRAME_SIZE_BYTES 1279                  /* Total frame size    */
#define FRAME_SIZE_BITS (FRAME_SIZE_BYTES * 8) /* Frame size in bits  */
#define SYNC_BITS 32                           /* Sync word length    */

/* *****************************************************************************
 *
 *                         DEFAULT PARAMETERS
 *
 * *****************************************************************************/

/* =============================================================================
 * DEFAULT CONFIGURATION VALUES
 * -----------------------------------------------------------------------------
 * These values are used when not overridden by command-line arguments.
 * Modify these for your specific signal parameters.
 * =============================================================================
 */

/* Input file path (change to your IQ file location) */
#define DEFAULT_INPUT_FILE \
  "C:/Users/4/Downloads/9082_Records/rx_80MHz_dec_counter_2_720.c16"

/* Decimation and sample rate */
#define DEFAULT_DECIM 5    /* Decimation factor                  */
#define DEFAULT_SPS 18.75f /* Samples per symbol (after decim)   */
#define DEFAULT_RB 160e6f  /* Symbol rate (baud)                 */

/* Costas loop parameters (carrier recovery) */
#define DEFAULT_COSTAS_ALPHA 0.01f  /* Proportional gain                  */
#define DEFAULT_COSTAS_BETA 0.0005f /* Integral gain                      */

/* Mueller & Müller timing loop parameters */
#define DEFAULT_TIMING_ALPHA 0.1f  /* Proportional gain                  */
#define DEFAULT_TIMING_BETA 0.005f /* Integral gain                      */

/* RRC matched filter settings */
#define DEFAULT_RRC_ENABLE 1     /* Enable RRC filter (0/1)            */
#define DEFAULT_RRC_ALPHA 0.8f   /* Roll-off factor (0.0 - 1.0)        */
#define DEFAULT_RRC_SPAN 12      /* Filter span in symbols             */
#define DEFAULT_RRC_TRIM_DELAY 0 /* Trim group delay (0/1)             */

/* ADC/Signal scaling */
#define DEFAULT_FS_VPP 1.475f /* Full-scale peak-to-peak voltage    */
#define DEFAULT_RLOAD 50.0f   /* Load resistance (ohms)             */
#define DEFAULT_ALPHA 0.35f   /* Signal scaling factor              */

/* EVM calculation window */
#define DEFAULT_EVM_SKIP_SYMS 5000   /* Symbols to skip at start           */
#define DEFAULT_EVM_LAST_SYMS 600000 /* Max symbols for EVM calculation    */

/* *****************************************************************************
 *
 *                         TYPE DEFINITIONS
 *
 * *****************************************************************************/

/* =============================================================================
 * COMPLEX NUMBER TYPE
 * -----------------------------------------------------------------------------
 * MSVC-compatible complex number structure (no C99 complex.h dependency)
 * =============================================================================
 */
typedef struct {
  float re; /* Real component      */
  float im; /* Imaginary component */
} cplxf;

/* =============================================================================
 * CONFIGURATION STRUCTURE
 * -----------------------------------------------------------------------------
 * Holds all demodulator configuration parameters
 * =============================================================================
 */
typedef struct {
  /* Input settings */
  char input_file[256]; /* Path to input IQ file                      */
  int input_format;     /* FMT_IQ16 or FMT_IQ32                       */
  int modulation;       /* MOD_OQPSK or MOD_BPSK                      */

  /* Sample rate control */
  int decim; /* Decimation factor                          */
  float sps; /* Samples per symbol                         */
  float rb;  /* Symbol rate (baud)                         */

  /* Costas loop (carrier recovery) */
  float costas_alpha; /* Proportional gain                          */
  float costas_beta;  /* Integral gain                              */

  /* Timing recovery (Mueller & Müller) */
  float timing_alpha; /* Proportional gain                          */
  float timing_beta;  /* Integral gain                              */

  /* RRC matched filter */
  int rrc_enable;     /* Enable RRC filter                          */
  float rrc_alpha;    /* Roll-off factor                            */
  int rrc_span;       /* Filter span (symbols)                      */
  int rrc_trim_delay; /* Trim group delay                           */

  /* Signal scaling */
  float fs_vpp; /* ADC full-scale Vpp                         */
  float rload;  /* Load resistance                            */
  float alpha;  /* Scaling factor                             */

  /* EVM calculation */
  int evm_skip_syms; /* Symbols to skip at start                   */
  int evm_last_syms; /* Max symbols for calculation                */
} Config;

/* =============================================================================
 * DYNAMIC BUFFER TYPES
 * -----------------------------------------------------------------------------
 * Growable arrays for signal processing
 * =============================================================================
 */
typedef struct {
  cplxf* data;     /* Buffer data pointer  */
  size_t len;      /* Current length       */
  size_t capacity; /* Allocated capacity   */
} SignalBuffer;

typedef struct {
  float* data;     /* Buffer data pointer  */
  size_t len;      /* Current length       */
  size_t capacity; /* Allocated capacity   */
} FloatBuffer;

/* *****************************************************************************
 *
 *                         EXTERNAL FUNCTION DECLARATIONS
 *
 * *****************************************************************************/

/* =============================================================================
 * BLIND PROCESSING FUNCTIONS (C linkage)
 * =============================================================================
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Viterbi decoder for convolutional codes
 * @param input    Encoded input bytes
 * @param output   Decoded output bytes
 * @param noutputitemsnumber  Number of output items
 */
void convolutional_decode_viterbi(unsigned char* input, unsigned char* output,
                                  int noutputitemsnumber);

/**
 * NRZ-M differential decoder
 * @param input   Input byte array
 * @param output  Output byte array
 * @param len     Number of bytes
 * @param inverse_decode  Decode direction flag
 */
void nrzm_decode(const unsigned char* input, unsigned char* output, int len,
                 int inverse_decode);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "Scrambler.h"
#endif

/* *****************************************************************************
 *
 *                         FUNCTION PROTOTYPES
 *
 * *****************************************************************************/

/* Configuration */
static void config_init_defaults(Config* cfg);
static void config_parse_args(Config* cfg, int argc, char** argv);
static void config_print(const Config* cfg);
static void print_usage(const char* prog_name);

/* Buffer management */
SignalBuffer* signal_buffer_create(size_t capacity);
void signal_buffer_free(SignalBuffer* buf);
void signal_buffer_append(SignalBuffer* buf, cplxf val);
FloatBuffer* float_buffer_create(size_t capacity);
void float_buffer_free(FloatBuffer* buf);
void float_buffer_append(FloatBuffer* buf, float val);

/* Complex number operations */
static inline cplxf cplxf_make(float re, float im);
static inline cplxf cplxf_add(cplxf a, cplxf b);
static inline cplxf cplxf_sub(cplxf a, cplxf b);
static inline cplxf cplxf_mul(cplxf a, cplxf b);
static inline cplxf cplxf_mul_scalar(cplxf a, float s);
static inline cplxf cplxf_div_scalar(cplxf a, float s);
static inline cplxf cplxf_conj(cplxf a);
static inline float cplxf_abs(cplxf a);
static inline float cplxf_abs2(cplxf a);
static inline cplxf cplxf_exp_i(float theta);
static inline cplxf cplxf_exp_i_d(double theta);
static inline float cplxf_real(cplxf a);
static inline float cplxf_imag(cplxf a);

/* Filter design and convolution */
float* rrc_taps(float fs_hz, float rs_hz, float alpha, int span_symbols,
                int* ntaps_out);
float* hamming_window_fir(float cutoff_norm, int ntaps);
cplxf* convolve_fir(cplxf* sig, size_t sig_len, float* taps, int ntaps);

/* QPSK symbol processing */
cplxf slicer_qpsk(cplxf z);
void qpsk_bits(cplxf sym, char* out);
float evm_decision_directed_qpsk(cplxf* syms, size_t n);

/* BPSK symbol processing */
float slicer_bpsk(float val);
char bpsk_bit(float sym);
float evm_decision_directed_bpsk(float* syms, size_t n);

/* Signal loading and processing */
cplxf* load_and_process(Config* cfg, size_t* out_len, float* final_sps,
                        double* pwr_raw_w, double* pwr_post_w);

/* Demodulation loops */
void run_loops(cplxf* sig, size_t N, float current_sps, Config* cfg,
               cplxf** costas_out, cplxf** syms_out, size_t* nsyms,
               float** freq_log, float** sps_log, size_t* nlog, int quiet);

void run_loops_bpsk(cplxf* sig, size_t N, float current_sps, Config* cfg,
                    cplxf** costas_out, float** syms_out, size_t* nsyms,
                    float** freq_log, float** sps_log, size_t* nlog, int quiet);

/* Blind processing chain */
int process_blind(const unsigned char* in, unsigned char* out, int in_len);
void bytesToBits(const unsigned char* input, unsigned char* output,
                 size_t byteCount);
void bitsToBytes(const unsigned char* input, unsigned char* output,
                 size_t bitCount);

/* Interpolation helpers */
static inline cplxf interpolate_sample(cplxf* buffer, size_t N, double pos);
static inline float interpolate_sample_f(float* buffer, size_t N, double pos);

/* *****************************************************************************
 *
 *                         CONFIGURATION FUNCTIONS
 *
 * *****************************************************************************/

/**
 * Initialize configuration with default values
 * @param cfg  Configuration structure to initialize
 */
static void config_init_defaults(Config* cfg) {
  /* Input settings */
  strncpy(cfg->input_file, DEFAULT_INPUT_FILE, sizeof(cfg->input_file) - 1);
  cfg->input_file[sizeof(cfg->input_file) - 1] = '\0';
  cfg->input_format = INPUT_FORMAT;
  cfg->modulation = MODULATION;

  /* Sample rate control */
  cfg->decim = DEFAULT_DECIM;
  cfg->sps = DEFAULT_SPS;
  cfg->rb = DEFAULT_RB;

  /* Costas loop */
  cfg->costas_alpha = DEFAULT_COSTAS_ALPHA;
  cfg->costas_beta = DEFAULT_COSTAS_BETA;

  /* Timing recovery */
  cfg->timing_alpha = DEFAULT_TIMING_ALPHA;
  cfg->timing_beta = DEFAULT_TIMING_BETA;

  /* RRC filter */
  cfg->rrc_enable = DEFAULT_RRC_ENABLE;
  cfg->rrc_alpha = DEFAULT_RRC_ALPHA;
  cfg->rrc_span = DEFAULT_RRC_SPAN;
  cfg->rrc_trim_delay = DEFAULT_RRC_TRIM_DELAY;

  /* Signal scaling */
  cfg->fs_vpp = DEFAULT_FS_VPP;
  cfg->rload = DEFAULT_RLOAD;
  cfg->alpha = DEFAULT_ALPHA;

  /* EVM settings */
  cfg->evm_skip_syms = DEFAULT_EVM_SKIP_SYMS;
  cfg->evm_last_syms = DEFAULT_EVM_LAST_SYMS;
}

/**
 * Parse command-line arguments into configuration
 * @param cfg   Configuration structure to update
 * @param argc  Argument count
 * @param argv  Argument vector
 */
static void config_parse_args(Config* cfg, int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      strncpy(cfg->input_file, argv[++i], sizeof(cfg->input_file) - 1);
      cfg->input_file[sizeof(cfg->input_file) - 1] = '\0';
    } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      cfg->decim = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--sps") == 0 && i + 1 < argc) {
      cfg->sps = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--costas-alpha") == 0 && i + 1 < argc) {
      cfg->costas_alpha = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--costas-beta") == 0 && i + 1 < argc) {
      cfg->costas_beta = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--timing-alpha") == 0 && i + 1 < argc) {
      cfg->timing_alpha = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--timing-beta") == 0 && i + 1 < argc) {
      cfg->timing_beta = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--rrc-alpha") == 0 && i + 1 < argc) {
      cfg->rrc_alpha = (float)atof(argv[++i]);
    } else if (strcmp(argv[i], "--rrc-span") == 0 && i + 1 < argc) {
      cfg->rrc_span = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--rrc_enable") == 0) {
      cfg->rrc_enable = 1;
    } else if (strcmp(argv[i], "--no-rrc") == 0) {
      cfg->rrc_enable = 0;
    } else if (strcmp(argv[i], "--bpsk") == 0) {
      cfg->modulation = MOD_BPSK;
    } else if (strcmp(argv[i], "--oqpsk") == 0) {
      cfg->modulation = MOD_OQPSK;
    } else if (strcmp(argv[i], "--iq16") == 0) {
      cfg->input_format = FMT_IQ16;
    } else if (strcmp(argv[i], "--iq32") == 0) {
      cfg->input_format = FMT_IQ32;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      exit(0);
    }
  }
}

/**
 * Print configuration summary
 * @param cfg  Configuration to print
 */
static void config_print(const Config* cfg) {
  printf("===========================================\n");
  printf("       OQPSK/BPSK File Demodulator\n");
  printf("===========================================\n");
  printf("\n[Input Settings]\n");
  printf("  File:         %s\n", cfg->input_file);
  printf("  Format:       %s\n",
         cfg->input_format == FMT_IQ32 ? "IQ32 (32-bit)" : "IQ16 (16-bit)");
  printf("  Modulation:   %s\n",
         cfg->modulation == MOD_BPSK ? "BPSK" : "OQPSK");

  printf("\n[Sample Rate]\n");
  printf("  Decimation:   %d\n", cfg->decim);
  printf("  SPS:          %.4f\n", cfg->sps);
  printf("  Symbol Rate:  %.3e baud\n", cfg->rb);

  printf("\n[Costas Loop]\n");
  printf("  Alpha:        %.6f\n", cfg->costas_alpha);
  printf("  Beta:         %.6f\n", cfg->costas_beta);

  printf("\n[Timing Recovery]\n");
  printf("  Alpha:        %.6f\n", cfg->timing_alpha);
  printf("  Beta:         %.6f\n", cfg->timing_beta);

  printf("\n[RRC Filter]\n");
  printf("  Enabled:      %s\n", cfg->rrc_enable ? "Yes" : "No");
  if (cfg->rrc_enable) {
    printf("  Roll-off:     %.2f\n", cfg->rrc_alpha);
    printf("  Span:         %d symbols\n", cfg->rrc_span);
  }

  printf("\n[Processing Toggles]\n");
  printf("  Low-pass:     %s\n", ENABLE_LOWPASS ? "ON" : "OFF");
  printf("  Convolution:  %s\n", ENABLE_CONVOLUTION ? "ON" : "OFF");
  printf("  NRZ-M:        %s\n", ENABLE_NRZM ? "ON" : "OFF");
  printf("  PSR:          %s\n", ENABLE_PSR ? "ON" : "OFF");
  printf("  Auto-tune:    %s\n", ENABLE_AUTO_TUNE ? "ON" : "OFF");
  printf("  UDP Sender:   %s\n", ENABLE_UDP_SENDER ? "ON" : "OFF");

  printf("===========================================\n\n");
}

/**
 * Print usage information
 * @param prog_name  Program name from argv[0]
 */
static void print_usage(const char* prog_name) {
  printf("OQPSK/BPSK File Demodulator\n\n");
  printf("Usage: %s [options]\n\n", prog_name);
  printf("Input Options:\n");
  printf("  -i FILE              Input IQ file path\n");
  printf("  --iq16               16-bit IQ format (default)\n");
  printf("  --iq32               32-bit IQ format\n");
  printf("\nModulation:\n");
  printf("  --oqpsk              OQPSK mode (default)\n");
  printf("  --bpsk               BPSK mode\n");
  printf("\nSample Rate:\n");
  printf("  -d NUM               Decimation factor\n");
  printf("  --sps NUM            Samples per symbol\n");
  printf("\nLoop Parameters:\n");
  printf("  --costas-alpha NUM   Costas loop proportional gain\n");
  printf("  --costas-beta NUM    Costas loop integral gain\n");
  printf("  --timing-alpha NUM   Timing loop proportional gain\n");
  printf("  --timing-beta NUM    Timing loop integral gain\n");
  printf("\nRRC Filter:\n");
  printf("  --rrc_enable         Enable RRC filter\n");
  printf("  --no-rrc             Disable RRC filter\n");
  printf("  --rrc-alpha NUM      RRC roll-off factor\n");
  printf("  --rrc-span NUM       RRC span in symbols\n");
  printf("\nOther:\n");
  printf("  -h, --help           Show this help message\n");
}

/* *****************************************************************************
 *
 *                         COMPLEX NUMBER OPERATIONS
 *
 * *****************************************************************************/

static inline cplxf cplxf_make(float re, float im) {
  cplxf c = {re, im};
  return c;
}

static inline cplxf cplxf_add(cplxf a, cplxf b) {
  cplxf c = {a.re + b.re, a.im + b.im};
  return c;
}

static inline cplxf cplxf_sub(cplxf a, cplxf b) {
  cplxf c = {a.re - b.re, a.im - b.im};
  return c;
}

static inline cplxf cplxf_mul(cplxf a, cplxf b) {
  cplxf c = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
  return c;
}

static inline cplxf cplxf_mul_scalar(cplxf a, float s) {
  cplxf c = {a.re * s, a.im * s};
  return c;
}

static inline cplxf cplxf_div_scalar(cplxf a, float s) {
  cplxf c = {a.re / s, a.im / s};
  return c;
}

static inline cplxf cplxf_conj(cplxf a) {
  cplxf c = {a.re, -a.im};
  return c;
}

static inline float cplxf_abs(cplxf a) {
  return sqrtf(a.re * a.re + a.im * a.im);
}

static inline float cplxf_abs2(cplxf a) { return a.re * a.re + a.im * a.im; }

static inline cplxf cplxf_exp_i(float theta) {
  cplxf c = {cosf(theta), sinf(theta)};
  return c;
}

static inline cplxf cplxf_exp_i_d(double theta) {
  cplxf c = {(float)cos(theta), (float)sin(theta)};
  return c;
}

static inline float cplxf_real(cplxf a) { return a.re; }

static inline float cplxf_imag(cplxf a) { return a.im; }
static double mean_power_w_cplxf(const cplxf* x, size_t N, double rload_ohm) {
  if (!x || N == 0 || rload_ohm <= 0.0) return 0.0;
  long double acc = 0.0L;
  for (size_t i = 0; i < N; i++) {
    long double re = x[i].re;
    long double im = x[i].im;
    acc += (re * re + im * im);
  }
  long double mean_v2 = acc / (long double)N;  // mean(|V|^2)
  long double p_w = mean_v2 / (long double)rload_ohm;
  return (double)p_w;
}

static double watt_to_dbm(double w) {
  if (!(w > 0.0)) return -INFINITY;
  return 10.0 * log10(w / 1e-3);
}

static double w_per_hz_to_dbm_per_hz(double w_per_hz) {
  if (!(w_per_hz > 0.0)) return -INFINITY;
  return 10.0 * log10(w_per_hz / 1e-3);
}

/* *****************************************************************************
 *
 *                         BUFFER MANAGEMENT
 *
 * *****************************************************************************/

/**
 * Create a signal buffer with initial capacity
 * @param capacity  Initial allocation size
 * @return Pointer to new SignalBuffer
 */
SignalBuffer* signal_buffer_create(size_t capacity) {
  SignalBuffer* buf = (SignalBuffer*)malloc(sizeof(SignalBuffer));
  buf->data = (cplxf*)malloc(capacity * sizeof(cplxf));
  buf->len = 0;
  buf->capacity = capacity;
  return buf;
}

/**
 * Free a signal buffer
 * @param buf  Buffer to free
 */
void signal_buffer_free(SignalBuffer* buf) {
  if (buf) {
    free(buf->data);
    free(buf);
  }
}

/**
 * Append a value to signal buffer (auto-grows)
 * @param buf  Target buffer
 * @param val  Value to append
 */
void signal_buffer_append(SignalBuffer* buf, cplxf val) {
  if (buf->len >= buf->capacity) {
    buf->capacity *= 2;
    buf->data = (cplxf*)realloc(buf->data, buf->capacity * sizeof(cplxf));
  }
  buf->data[buf->len++] = val;
}

/**
 * Create a float buffer with initial capacity
 * @param capacity  Initial allocation size
 * @return Pointer to new FloatBuffer
 */
FloatBuffer* float_buffer_create(size_t capacity) {
  FloatBuffer* buf = (FloatBuffer*)malloc(sizeof(FloatBuffer));
  buf->data = (float*)malloc(capacity * sizeof(float));
  buf->len = 0;
  buf->capacity = capacity;
  return buf;
}

/**
 * Free a float buffer
 * @param buf  Buffer to free
 */
void float_buffer_free(FloatBuffer* buf) {
  if (buf) {
    free(buf->data);
    free(buf);
  }
}

/**
 * Append a value to float buffer (auto-grows)
 * @param buf  Target buffer
 * @param val  Value to append
 */
void float_buffer_append(FloatBuffer* buf, float val) {
  if (buf->len >= buf->capacity) {
    buf->capacity *= 2;
    buf->data = (float*)realloc(buf->data, buf->capacity * sizeof(float));
  }
  buf->data[buf->len++] = val;
}

/* *****************************************************************************
 *
 *                         FILTER DESIGN
 *
 * *****************************************************************************/

/**
 * Generate Root Raised Cosine filter taps
 *
 * @param fs_hz        Sample rate in Hz
 * @param rs_hz        Symbol rate in Hz
 * @param alpha        Roll-off factor (0.0 - 1.0)
 * @param span_symbols Filter span in symbols
 * @param ntaps_out    Output: number of taps generated
 * @return Pointer to tap array (caller must free)
 */
float* rrc_taps(float fs_hz, float rs_hz, float alpha, int span_symbols,
                int* ntaps_out) {
  float Ts = 1.0f / rs_hz;
  float sps = fs_hz / rs_hz;
  int ntaps = (int)ceilf(span_symbols * sps);

  /* Ensure odd number of taps for symmetry */
  if (ntaps % 2 == 0) ntaps++;

  float* h = (float*)malloc(ntaps * sizeof(float));

  for (int n = 0; n < ntaps; n++) {
    float tn = (n - ntaps / 2) / fs_hz;

    if (fabsf(tn) < 1e-12f) {
      /* t = 0 case */
      h[n] = (1.0f + alpha * (4.0f / (float)M_PI - 1.0f)) / sqrtf(Ts);
    } else if (alpha > 0 && fabsf(fabsf(tn) - Ts / (4.0f * alpha)) < 1e-12f) {
      /* t = ±Ts/(4α) case */
      float a = alpha;
      h[n] = (a / sqrtf(2.0f * Ts)) *
             ((1.0f + 2.0f / (float)M_PI) * sinf((float)M_PI / (4.0f * a)) +
              (1.0f - 2.0f / (float)M_PI) * cosf((float)M_PI / (4.0f * a)));
    } else {
      /* General case */
      float x = (float)M_PI * tn / Ts;
      float num = sinf(x * (1.0f - alpha)) +
                  4.0f * alpha * (tn / Ts) * cosf(x * (1.0f + alpha));
      float den = x * (1.0f - powf(4.0f * alpha * tn / Ts, 2.0f));
      h[n] = (num / den) / sqrtf(Ts);
    }
  }

  /* Normalize for unit energy */
  float energy = 0.0f;
  for (int n = 0; n < ntaps; n++) {
    energy += h[n] * h[n];
  }
  energy = sqrtf(energy + 1e-30f);
  for (int n = 0; n < ntaps; n++) {
    h[n] /= energy;
  }

  *ntaps_out = ntaps;
  return h;
}

/**
 * Generate Hamming-windowed FIR low-pass filter
 *
 * @param cutoff_norm  Normalized cutoff frequency (0.0 - 0.5)
 * @param ntaps        Number of filter taps
 * @return Pointer to tap array (caller must free)
 */
float* hamming_window_fir(float cutoff_norm, int ntaps) {
  float* h = (float*)malloc(ntaps * sizeof(float));
  int M = ntaps - 1;

  for (int n = 0; n < ntaps; n++) {
    /* Hamming window */
    float w = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * n / M);

    /* Sinc function */
    float x = n - M / 2.0f;
    float sinc =
        (fabsf(x) < 1e-7f)
            ? 1.0f
            : sinf(2.0f * (float)M_PI * cutoff_norm * x) / ((float)M_PI * x);

    h[n] = 2.0f * cutoff_norm * sinc * w;
  }

  /* Normalize for unity DC gain */
  float sum = 0.0f;
  for (int n = 0; n < ntaps; n++) {
    sum += h[n];
  }
  for (int n = 0; n < ntaps; n++) {
    h[n] /= sum;
  }

  return h;
}

/**
 * Apply FIR filter to complex signal
 *
 * @param sig      Input signal array
 * @param sig_len  Signal length
 * @param taps     Filter coefficients
 * @param ntaps    Number of taps
 * @return Filtered signal array (caller must free)
 */
cplxf* convolve_fir(cplxf* sig, size_t sig_len, float* taps, int ntaps) {
  cplxf* out = (cplxf*)malloc(sig_len * sizeof(cplxf));
  int delay = ntaps / 2;

  for (size_t n = 0; n < sig_len; n++) {
    cplxf acc = cplxf_make(0.0f, 0.0f);
    for (int k = 0; k < ntaps; k++) {
      int idx = (int)n - delay + k;
      if (idx >= 0 && idx < (int)sig_len) {
        acc = cplxf_add(acc, cplxf_mul_scalar(sig[idx], taps[k]));
      }
    }
    out[n] = acc;
  }

  return out;
}

/* *****************************************************************************
 *
 *                         SYMBOL PROCESSING - QPSK
 *
 * *****************************************************************************/

/**
 * QPSK hard decision slicer
 * Maps complex symbol to nearest constellation point (±1, ±1)
 *
 * @param z  Input symbol
 * @return Sliced symbol
 */
cplxf slicer_qpsk(cplxf z) {
  float re = z.re >= 0 ? 1.0f : -1.0f;
  float im = z.im >= 0 ? 1.0f : -1.0f;
  return cplxf_make(re, im);
}

/**
 * Convert QPSK symbol to bit string
 *
 * @param sym  Input symbol
 * @param out  Output buffer (at least 3 chars)
 */
void qpsk_bits(cplxf sym, char* out) {
  out[0] = sym.re >= 0 ? '1' : '0';
  out[1] = sym.im >= 0 ? '1' : '0';
  out[2] = '\0';
}

/**
 * Calculate decision-directed EVM for QPSK symbols
 *
 * @param syms  Symbol array
 * @param n     Number of symbols
 * @return EVM as fraction (0.0 - 1.0)
 */
float evm_decision_directed_qpsk(cplxf* syms, size_t n) {
  /* Estimate complex gain */
  cplxf num = cplxf_make(0.0f, 0.0f);
  float den = 0.0f;

  for (size_t i = 0; i < n; i++) {
    cplxf ref = slicer_qpsk(syms[i]);
    num = cplxf_add(num, cplxf_mul(cplxf_conj(ref), syms[i]));
    den += cplxf_abs2(ref);
  }

  cplxf a = cplxf_div_scalar(num, den + 1e-30f);

  /* Calculate EVM */
  float err_sum = 0.0f, ref_sum = 0.0f;

  for (size_t i = 0; i < n; i++) {
    cplxf ref = slicer_qpsk(syms[i]);
    cplxf e = cplxf_sub(syms[i], cplxf_mul(a, ref));
    cplxf a_ref = cplxf_mul(a, ref);
    err_sum += cplxf_abs2(e);
    ref_sum += cplxf_abs2(a_ref);
  }

  return sqrtf(err_sum / (ref_sum + 1e-30f));
}

/* *****************************************************************************
 *
 *                         SYMBOL PROCESSING - BPSK
 *
 * *****************************************************************************/

/**
 * BPSK hard decision slicer
 *
 * @param val  Input sample
 * @return +1.0 or -1.0
 */
float slicer_bpsk(float val) { return val >= 0 ? 1.0f : -1.0f; }

/**
 * Convert BPSK symbol to bit character
 *
 * @param sym  Input symbol
 * @return '1' or '0'
 */
char bpsk_bit(float sym) { return sym >= 0 ? '1' : '0'; }

/**
 * Calculate decision-directed EVM for BPSK symbols
 *
 * @param syms  Symbol array
 * @param n     Number of symbols
 * @return EVM as fraction (0.0 - 1.0)
 */
float evm_decision_directed_bpsk(float* syms, size_t n) {
  /* Estimate gain */
  float num = 0.0f, den = 0.0f;

  for (size_t i = 0; i < n; i++) {
    float ref = slicer_bpsk(syms[i]);
    num += ref * syms[i];
    den += ref * ref;
  }

  float a = num / (den + 1e-30f);

  /* Calculate EVM */
  float err_sum = 0.0f, ref_sum = 0.0f;

  for (size_t i = 0; i < n; i++) {
    float ref = slicer_bpsk(syms[i]);
    float e = syms[i] - a * ref;
    float a_ref = a * ref;
    err_sum += e * e;
    ref_sum += a_ref * a_ref;
  }

  return sqrtf(err_sum / (ref_sum + 1e-30f));
}

/* *****************************************************************************
 *
 *                         INTERPOLATION HELPERS
 *
 * *****************************************************************************/

/**
 * Linear interpolation for complex samples
 *
 * @param buffer  Sample buffer
 * @param N       Buffer length
 * @param pos     Fractional position
 * @return Interpolated sample
 */
static inline cplxf interpolate_sample(cplxf* buffer, size_t N, double pos) {
  cplxf val = cplxf_make(0.0f, 0.0f);

  if (pos >= 0.0 && pos < (double)N - 1.0) {
    size_t pi = (size_t)pos;
    double pf = pos - (double)pi;
    float pff = (float)pf;
    val = cplxf_add(cplxf_mul_scalar(buffer[pi], 1.0f - pff),
                    cplxf_mul_scalar(buffer[pi + 1], pff));
  }

  return val;
}

/**
 * Linear interpolation for float samples
 *
 * @param buffer  Sample buffer
 * @param N       Buffer length
 * @param pos     Fractional position
 * @return Interpolated sample
 */
static inline float interpolate_sample_f(float* buffer, size_t N, double pos) {
  float val = 0.0f;

  if (pos >= 0.0 && pos < (double)N - 1.0) {
    size_t pi = (size_t)pos;
    double pf = pos - (double)pi;
    float pff = (float)pf;
    val = buffer[pi] * (1.0f - pff) + buffer[pi + 1] * pff;
  }

  return val;
}

/* *****************************************************************************
 *
 *                         SIGNAL LOADING AND PREPROCESSING
 *
 * *****************************************************************************/

/**
 * Load IQ file and apply preprocessing chain
 *
 * Processing steps:
 *   1. Load raw IQ samples
 *   2. Remove DC offset
 *   3. Scale to voltage
 *   4. Optional low-pass filtering
 *   5. Decimation
 *   6. Optional RRC matched filtering
 *   7. Normalize amplitude
 *
 * @param cfg        Configuration parameters
 * @param out_len    Output: number of samples after processing
 * @param final_sps  Output: effective samples per symbol
 * @return Processed signal array (caller must free)
 */
cplxf* load_and_process(Config* cfg, size_t* out_len, float* final_sps,
                        double* pwr_raw_w, double* pwr_post_w){

  if (pwr_raw_w) *pwr_raw_w = 0.0;
  if (pwr_post_w) *pwr_post_w = 0.0;
  printf("--- STEP 1: LOADING & DECIMATION ---\n");

  /* Open input file */
  FILE* f = fopen(cfg->input_file, "rb");
  if (!f) {
    fprintf(stderr, "Error: Cannot open input file: %s\n", cfg->input_file);
    return NULL;
  }

  /* Get file size */
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  size_t n_samples;
  float* i_arr;
  float* q_arr;

  /* Load samples based on format */
  if (cfg->input_format == FMT_IQ32) {
    /* IQ32: 32-bit I, 32-bit Q (int32_t pairs) */
    n_samples = file_size / (2 * sizeof(int32_t));
    int32_t* raw_int = (int32_t*)malloc(2 * n_samples * sizeof(int32_t));
    fread(raw_int, sizeof(int32_t), 2 * n_samples, f);
    fclose(f);

    printf("   Loaded %zu IQ samples (IQ32 format)\n", n_samples);

    i_arr = (float*)malloc(n_samples * sizeof(float));
    q_arr = (float*)malloc(n_samples * sizeof(float));
    for (size_t n = 0; n < n_samples; n++) {
      i_arr[n] = (float)raw_int[2 * n];
      q_arr[n] = (float)raw_int[2 * n + 1];
    }
    free(raw_int);
  } else {
    /* IQ16: 16-bit I, 16-bit Q (int16_t pairs) - default */
    n_samples = file_size / (2 * sizeof(int16_t));
    int16_t* raw_int = (int16_t*)malloc(2 * n_samples * sizeof(int16_t));
    fread(raw_int, sizeof(int16_t), 2 * n_samples, f);
    fclose(f);

    printf("   Loaded %zu IQ samples (IQ16 format)\n", n_samples);

    i_arr = (float*)malloc(n_samples * sizeof(float));
    q_arr = (float*)malloc(n_samples * sizeof(float));
    for (size_t n = 0; n < n_samples; n++) {
      i_arr[n] = (float)raw_int[2 * n];
      q_arr[n] = (float)raw_int[2 * n + 1];
    }
    free(raw_int);
  }

  /* Remove DC offset */
  float i_mean = 0.0f, q_mean = 0.0f;
  for (size_t n = 0; n < n_samples; n++) {
    i_mean += i_arr[n];
    q_mean += q_arr[n];
  }
  i_mean /= n_samples;
  q_mean /= n_samples;

  for (size_t n = 0; n < n_samples; n++) {
    i_arr[n] -= i_mean;
    q_arr[n] -= q_mean;
  }

  /* Scale to voltage */
  float Vpk = cfg->fs_vpp / 2.0f;
  float v_per_count;
  if (cfg->input_format == FMT_IQ32) {
    v_per_count = Vpk / 2147483648.0f; /* 2^31 for 32-bit */
  } else {
    v_per_count = Vpk / 32768.0f; /* 2^15 for 16-bit */
  }

  cplxf* sig_v = (cplxf*)malloc(n_samples * sizeof(cplxf));
  for (size_t n = 0; n < n_samples; n++) {
    sig_v[n] = cplxf_make(i_arr[n] * v_per_count, q_arr[n] * v_per_count);
  }
  free(i_arr);
  free(q_arr);
  // ---- RAW POWER (before any filtering/decim/normalize) ----
  if (pwr_raw_w) {
    *pwr_raw_w = mean_power_w_cplxf(sig_v, n_samples, (double)cfg->rload);
  }
  /* Optional low-pass filtering */
  cplxf* sig_filt;

#if ENABLE_LOWPASS
  float cutoff_norm = LOWPASS_CUTOFF_NORM / 150.0f;
  if (cutoff_norm > 0.45f) cutoff_norm = 0.45f;
  printf("   [FILTER] Low-pass enabled - Cutoff: %.4f (Fs), Taps: %d\n",
         cutoff_norm, LOWPASS_NTAPS);

  float* lp_taps = hamming_window_fir(cutoff_norm, LOWPASS_NTAPS);
  sig_filt = convolve_fir(sig_v, n_samples, lp_taps, LOWPASS_NTAPS);
  free(lp_taps);
  free(sig_v);
#else
  printf("   [FILTER] Low-pass disabled - skipping\n");
  sig_filt = sig_v;
#endif

  /* Decimation */
  size_t out_samples;
  cplxf* sig_dec;

  if (cfg->decim > 1) {
    out_samples = n_samples / cfg->decim;
    sig_dec = (cplxf*)malloc(out_samples * sizeof(cplxf));
    for (size_t n = 0; n < out_samples; n++) {
      sig_dec[n] = sig_filt[n * cfg->decim];
    }
    *final_sps = cfg->sps / cfg->decim;
    printf("   [DECIMATION] Factor: %d | New SPS: %.4f\n", cfg->decim,
           *final_sps);
    free(sig_filt);
  } else {
    sig_dec = sig_filt;
    out_samples = n_samples;
    *final_sps = cfg->sps;
  }

  /* Optional RRC matched filtering */
  cplxf* sig_out = sig_dec;
  size_t final_len = out_samples;

  if (cfg->rrc_enable) {
    float Rs = cfg->rb / 2.0f;
    float Fs_dec = (Rs * cfg->sps) / cfg->decim;
    int rrc_ntaps;
    float* rrc =
        rrc_taps(Fs_dec, Rs, cfg->rrc_alpha, cfg->rrc_span, &rrc_ntaps);
    cplxf* sig_rrc = convolve_fir(sig_dec, out_samples, rrc, rrc_ntaps);
    free(rrc);
    free(sig_dec);

    if (cfg->rrc_trim_delay) {
      int gd = (rrc_ntaps - 1) / 2;
      final_len = out_samples - gd;
      sig_out = (cplxf*)malloc(final_len * sizeof(cplxf));
      memcpy(sig_out, sig_rrc + gd, final_len * sizeof(cplxf));
      free(sig_rrc);
    } else {
      sig_out = sig_rrc;
      final_len = out_samples;
    }
  }
  if (pwr_post_w) {
    *pwr_post_w = mean_power_w_cplxf(sig_out, final_len, (double)cfg->rload);
  }
  /* Normalize amplitude */
  float max_abs = 0.0f;
  for (size_t n = 0; n < final_len; n++) {
    float mag = cplxf_abs(sig_out[n]);
    if (mag > max_abs) max_abs = mag;
  }
  for (size_t n = 0; n < final_len; n++) {
    sig_out[n] = cplxf_div_scalar(sig_out[n], max_abs + 1e-12f);
  }

  *out_len = final_len;
  return sig_out;
}

/* *****************************************************************************
 *
 *                         BIT/BYTE CONVERSION UTILITIES
 *
 * *****************************************************************************/

/**
 * Convert packed bytes to unpacked bits (MSB first)
 *
 * @param input      Input byte array
 * @param output     Output bit array
 * @param byteCount  Number of bytes to convert
 */
void bytesToBits(const unsigned char* input, unsigned char* output,
                 size_t byteCount) {
  for (size_t i = 0; i < byteCount; i++) {
    for (int j = 0; j < 8; j++) {
      output[i * 8 + (7 - j)] = (input[i] >> j) & 1;
    }
  }
}

/**
 * Convert unpacked bits to packed bytes (MSB first)
 *
 * @param input     Input bit array
 * @param output    Output byte array
 * @param bitCount  Number of bits to convert
 */
void bitsToBytes(const unsigned char* input, unsigned char* output,
                 size_t bitCount) {
  for (size_t i = 0; i < (bitCount / 8); i++) {
    output[i] = 0;
    for (int j = 0; j < 8; j++) {
      output[i] |= ((int)input[i * 8 + j]) << (7 - j);
    }
  }
}

/* *****************************************************************************
 *
 *                         UDP STREAMING
 *
 * *****************************************************************************/

#if ENABLE_UDP_SENDER
/**
 * Send demodulated bits via UDP (unpacked format: 0x01/0x00)
 *
 * @param bits        Bit array to send
 * @param total_bits  Total number of bits
 * @param host        Destination IP address
 * @param port        Destination UDP port
 * @param chunk_size  Bytes per UDP packet
 * @return 0 on success, -1 on error
 */
int udp_send_unpacked_bits(const unsigned char* bits, size_t total_bits,
                           const char* host, int port, size_t chunk_size) {
#ifdef _WIN32
  WSADATA wsa;
  SOCKET sock;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    fprintf(stderr, "[UDP] WSAStartup failed\n");
    return -1;
  }
#else
  int sock;
#endif

  struct sockaddr_in dest_addr;

  /* Create socket */
#ifdef _WIN32
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET) {
    fprintf(stderr, "[UDP] Socket creation failed\n");
    WSACleanup();
    return -1;
  }
#else
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    fprintf(stderr, "[UDP] Socket creation failed\n");
    return -1;
  }
#endif

  /* Setup destination address */
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(port);

#ifdef _WIN32
  dest_addr.sin_addr.S_un.S_addr = inet_addr(host);
#else
  inet_pton(AF_INET, host, &dest_addr.sin_addr);
#endif

  printf("\n--- UDP SENDER ---\n");
  printf("   Destination: %s:%d\n", host, port);
  printf("   Chunk size: %zu bytes (unpacked)\n", chunk_size);
  printf("   Total bits: %zu\n", total_bits);

  /* Allocate send buffer */
  unsigned char* unpacked_buffer = (unsigned char*)malloc(chunk_size);
  if (!unpacked_buffer) {
    fprintf(stderr, "[UDP] Memory allocation failed\n");
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return -1;
  }

  size_t total_sent = 0;
  size_t packet_count = 0;

  /* Send loop */
  for (size_t i = 0; i < total_bits; i += chunk_size) {
    size_t bits_to_send =
        (i + chunk_size <= total_bits) ? chunk_size : (total_bits - i);

    /* Convert bits to unpacked bytes (0x01 or 0x00) */
    for (size_t j = 0; j < bits_to_send; j++) {
      unpacked_buffer[j] = bits[i + j] ? 0x01 : 0x00;
    }

    /* Send UDP packet */
    int sent = sendto(sock, (const char*)unpacked_buffer, (int)bits_to_send, 0,
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    if (sent < 0) {
      fprintf(stderr, "[UDP] Send failed on packet %zu\n", packet_count);
    } else {
      total_sent += sent;
      packet_count++;
      if (packet_count % 100 == 0) {
        printf("   Sent %zu packets, %zu bytes\n", packet_count, total_sent);
      }
    }

    /* Inter-packet delay */
#ifdef _WIN32
    Sleep(UDP_DELAY_MS);
#else
    usleep(UDP_DELAY_MS * 1000);
#endif
  }

  printf("   COMPLETE: %zu packets, %zu bytes sent\n", packet_count,
         total_sent);

  free(unpacked_buffer);

#ifdef _WIN32
  closesocket(sock);
  WSACleanup();
#else
  close(sock);
#endif

  return 0;
}
#endif /* ENABLE_UDP_SENDER */

/* *****************************************************************************
 *
 *                         BLIND PROCESSING CHAIN
 *
 * *****************************************************************************/

/**
 * Apply blind processing chain: Viterbi -> NRZ-M -> PSR
 *
 * @param in       Input bit array (unpacked)
 * @param out      Output bit array (unpacked)
 * @param in_len   Number of input bits
 * @return Number of output bits, or -1 on error
 */
int process_blind(const unsigned char* in, unsigned char* out, int in_len) {
  int current_len = in_len;

  /* Allocate working buffers */
  unsigned char* temp1 = (unsigned char*)malloc(in_len);
  unsigned char* temp2 = (unsigned char*)malloc(in_len);
  unsigned char* current = (unsigned char*)malloc(in_len);

  if (!temp1 || !temp2 || !current) {
    printf("Error: Memory allocation failed\n");
    free(temp1);
    free(temp2);
    free(current);
    return -1;
  }

  memcpy(current, in, in_len);

  /* Stage 1: Convolutional (Viterbi) decoding */
#if ENABLE_CONVOLUTION
  printf("  [CONV] Enabled - decoding...\n");
  int packed_len = current_len / 8;
  unsigned char* packed_input = (unsigned char*)malloc(packed_len);
  unsigned char* conv_output = (unsigned char*)malloc(packed_len / 2);
  bitsToBytes(current, packed_input, current_len);
  convolutional_decode_viterbi(packed_input, conv_output, packed_len);
  current_len = packed_len / 2;
  memcpy(temp1, conv_output, current_len);
  free(packed_input);
  free(conv_output);
#else
  printf("  [CONV] Disabled - skipping\n");
  bitsToBytes(current, temp1, current_len);
  current_len = current_len / 8;
#endif

  /* Stage 2: NRZ-M differential decoding */
#if ENABLE_NRZM
  printf("  [NRZM] Enabled - decoding...\n");
  nrzm_decode(temp1, temp2, current_len, 0);
  memcpy(temp1, temp2, current_len);
#else
  printf("  [NRZM] Disabled - skipping\n");
#endif

  /* Stage 3: PSR (pseudo-random) descrambling */
#if ENABLE_PSR
  printf("  [PSR]  Enabled - descrambling...\n");
#ifdef __cplusplus
  Scrambler s;
  s.Scramble(&temp1[4]);
#else
  printf("  [PSR]  Warning: PSR requires C++, skipping...\n");
#endif
#else
  printf("  [PSR]  Disabled - skipping\n");
#endif

  /* Convert back to bits */
  int out_len = current_len * 8;
  bytesToBits(temp1, out, current_len);

  free(temp1);
  free(temp2);
  free(current);

  return out_len;
}

/* *****************************************************************************
 *
 *                         DEMODULATION LOOPS - BPSK
 *
 * *****************************************************************************/

/**
 * Run BPSK demodulation with Costas carrier recovery and M&M timing recovery
 *
 * @param sig          Input signal array
 * @param N            Signal length
 * @param current_sps  Samples per symbol
 * @param cfg          Configuration parameters
 * @param costas_out   Output: carrier-corrected signal (caller frees)
 * @param syms_out     Output: recovered symbols (caller frees)
 * @param nsyms        Output: number of symbols
 * @param freq_log     Output: frequency log (caller frees, NULL if quiet)
 * @param sps_log      Output: SPS log (caller frees, NULL if quiet)
 * @param nlog         Output: log length
 * @param quiet        If non-zero, suppress output and skip logging
 */
void run_loops_bpsk(cplxf* sig, size_t N, float current_sps, Config* cfg,
                    cplxf** costas_out, float** syms_out, size_t* nsyms,
                    float** freq_log, float** sps_log, size_t* nlog,
                    int quiet) {
  if (!quiet) {
    printf("\n--- STEP 2: RUNNING BPSK LOOPS (Costas + Mueller) ---\n");
  }

  cplxf* costas_buf = (cplxf*)malloc(N * sizeof(cplxf));
  int save_costas = !quiet;

  if (save_costas) {
    *freq_log = (float*)malloc(N * sizeof(float));
  } else {
    *freq_log = NULL;
  }

  /*
   * BPSK Costas Loop - Carrier Recovery
   * Phase detector: sign(I) * Q
   */
  double phase = 0.0, freq = 0.0;

  for (size_t k = 0; k < N; k++) {
    cplxf out = cplxf_mul(sig[k], cplxf_exp_i_d(-phase));

    /* BPSK phase error detector */
    double err = (double)copysignf(1.0f, out.re) * (double)out.im;

    /* Update loop filter */
    freq += (double)cfg->costas_beta * err;
    phase += freq + (double)cfg->costas_alpha * err;

    /* Wrap phase */
    while (phase > M_PI) phase -= 2.0 * M_PI;
    while (phase < -M_PI) phase += 2.0 * M_PI;

    costas_buf[k] = out;
    if (save_costas) (*freq_log)[k] = (float)freq;
  }

  *costas_out = costas_buf;

  /* Extract I channel for timing recovery */
  float* i_channel = (float*)malloc(N * sizeof(float));
  for (size_t k = 0; k < N; k++) {
    i_channel[k] = costas_buf[k].re;
  }

  /*
   * Mueller & Müller Timing Loop - Symbol Recovery
   */
  double sps_est = (double)current_sps;
  double idx = sps_est;
  size_t initial_capacity = quiet ? 50000 : 100000;

  FloatBuffer* sym_buf = float_buffer_create(initial_capacity);
  FloatBuffer* sps_buf =
      save_costas ? float_buffer_create(initial_capacity) : NULL;

  float prev_sym = 0.0f, prev_dec = 0.0f;
  int first = 1;

  const double sps_nom = (double)current_sps;
  const double sps_min = 0.50 * sps_nom;
  const double sps_max = 1.50 * sps_nom;
  const double min_step = 0.10; /* Minimum forward progress */

  /* Safety limit to prevent infinite loops */
  size_t max_iters = (size_t)((double)N / fmax(sps_nom, 1e-6)) * 4 + 1000;
  size_t iters = 0;

  while (idx < (double)N - sps_est - 5.0) {
    if (++iters > max_iters) {
      break; /* Bad parameter combo - bail out */
    }

    float sym = interpolate_sample_f(i_channel, N, idx);
    float_buffer_append(sym_buf, sym);

    if (first) {
      prev_sym = sym;
      prev_dec = slicer_bpsk(sym);
      first = 0;
      if (save_costas) float_buffer_append(sps_buf, (float)sps_est);
      idx += sps_est;
      continue;
    }

    float dec = slicer_bpsk(sym);

    /* M&M timing error detector for BPSK */
    double err =
        (double)prev_dec * (double)sym - (double)dec * (double)prev_sym;

    if (!isfinite(err) || !isfinite(sps_est) || !isfinite(idx)) {
      break;
    }

    /* Update SPS estimate */
    sps_est += (double)cfg->timing_beta * err;

    /* Clamp SPS */
    if (sps_est < sps_min) sps_est = sps_min;
    if (sps_est > sps_max) sps_est = sps_max;

    /* Calculate step */
    double step = sps_est + (double)cfg->timing_alpha * err;
    if (!isfinite(step) || step < min_step) step = min_step;
    idx += step;

    if (save_costas) float_buffer_append(sps_buf, (float)sps_est);

    prev_sym = sym;
    prev_dec = dec;

    if (!isfinite(idx) || !isfinite(sps_est)) {
      printf("Timing loop blew up (idx/sps not finite). Breaking.\n");
      break;
    }
  }

  printf("timing continues %f \n", sps_est);

  free(i_channel);

  *syms_out = sym_buf->data;
  *nsyms = sym_buf->len;
  free(sym_buf);

  if (save_costas) {
    *sps_log = sps_buf->data;
    *nlog = sps_buf->len;
    free(sps_buf);
  } else {
    *sps_log = NULL;
    *nlog = 0;
  }
}

/* *****************************************************************************
 *
 *                         DEMODULATION LOOPS - OQPSK
 *
 * *****************************************************************************/

/**
 * Run OQPSK demodulation with Costas carrier recovery and M&M timing recovery
 *
 * @param sig          Input signal array
 * @param N            Signal length
 * @param current_sps  Samples per symbol
 * @param cfg          Configuration parameters
 * @param costas_out   Output: carrier-corrected signal (caller frees)
 * @param syms_out     Output: recovered symbols (caller frees)
 * @param nsyms        Output: number of symbols
 * @param freq_log     Output: frequency log (caller frees, NULL if quiet)
 * @param sps_log      Output: SPS log (caller frees, NULL if quiet)
 * @param nlog         Output: log length
 * @param quiet        If non-zero, suppress output and skip logging
 */
void run_loops(cplxf* sig, size_t N, float current_sps, Config* cfg,
               cplxf** costas_out, cplxf** syms_out, size_t* nsyms,
               float** freq_log, float** sps_log, size_t* nlog, int quiet) {
  if (!quiet) {
    printf("\n--- STEP 2: RUNNING OQPSK LOOPS (Costas + Mueller) ---\n");
  }

  cplxf* costas_buf = (cplxf*)malloc(N * sizeof(cplxf));
  int save_costas = !quiet;

  if (save_costas) {
    *freq_log = (float*)malloc(N * sizeof(float));
  } else {
    *freq_log = NULL;
  }

  /*
   * QPSK Costas Loop - Carrier Recovery
   * Phase detector: sign(I)*Q - sign(Q)*I
   */
  double phase = 0.0, freq = 0.0;

  for (size_t k = 0; k < N; k++) {
    cplxf out = cplxf_mul(sig[k], cplxf_exp_i_d(-phase));

    /* QPSK phase error detector */
    double err = (double)copysignf(1.0f, out.re) * (double)out.im -
                 (double)copysignf(1.0f, out.im) * (double)out.re;

    /* Update loop filter */
    freq += (double)cfg->costas_beta * err;
    phase += freq + (double)cfg->costas_alpha * err;

    /* Wrap phase */
    while (phase > M_PI) phase -= 2.0 * M_PI;
    while (phase < -M_PI) phase += 2.0 * M_PI;

    costas_buf[k] = out;
    if (save_costas) (*freq_log)[k] = (float)freq;
  }

  *costas_out = costas_buf;

  /*
   * Mueller & Müller Timing Loop - Symbol Recovery
   * For OQPSK: I and Q sampled with half-symbol offset
   */
  double sps_est = (double)current_sps;
  double idx = 0.0;
  size_t initial_capacity = quiet ? 50000 : 100000;

  SignalBuffer* sym_buf = signal_buffer_create(initial_capacity);
  FloatBuffer* sps_buf =
      save_costas ? float_buffer_create(initial_capacity) : NULL;

  int first = 1;
  cplxf prev_sym = cplxf_make(0.0f, 0.0f);
  cplxf prev_dec = cplxf_make(0.0f, 0.0f);
  const float sps_nom = current_sps;

  while (idx < (double)N - sps_est - 5.0) {
    /* OQPSK: I and Q with half-symbol offset */
    cplxf i_sample = interpolate_sample(costas_buf, N, idx);
    cplxf q_sample = interpolate_sample(costas_buf, N, idx + sps_est / 2.0);
    cplxf sym = cplxf_make(i_sample.re, q_sample.im);

    signal_buffer_append(sym_buf, sym);

    if (first) {
      prev_sym = sym;
      prev_dec = slicer_qpsk(sym);
      first = 0;
      if (save_costas) float_buffer_append(sps_buf, (float)sps_est);
      idx += sps_est;
      continue;
    }

    cplxf dec = slicer_qpsk(sym);

    /* M&M timing error detector */
    double term1 = (double)prev_dec.re * (double)sym.re +
                   (double)prev_dec.im * (double)sym.im;
    double term2 = (double)dec.re * (double)prev_sym.re +
                   (double)dec.im * (double)prev_sym.im;
    double err = term1 - term2;

    /* Update SPS estimate */
    sps_est += (double)cfg->timing_beta * err;

    /* Clamp SPS */
    double sps_min = 0.5 * (double)sps_nom;
    double sps_max = 1.5 * (double)sps_nom;
    if (sps_est < sps_min) sps_est = sps_min;
    if (sps_est > sps_max) sps_est = sps_max;

    /* Calculate step */
    double step = sps_est + (double)cfg->timing_alpha * err;
    if (step < 0.10) step = 0.10;
    idx += step;

    if (save_costas) float_buffer_append(sps_buf, (float)sps_est);

    prev_sym = sym;
    prev_dec = dec;
  }

  *syms_out = sym_buf->data;
  *nsyms = sym_buf->len;
  free(sym_buf);

  if (save_costas) {
    *sps_log = sps_buf->data;
    *nlog = sps_buf->len;
    free(sps_buf);
  } else {
    *sps_log = NULL;
    *nlog = 0;
  }
}

/* *****************************************************************************
 *
 *                         MAIN ENTRY POINT
 *
 * *****************************************************************************/

int main(int argc, char** argv) {
  /* Initialize configuration with defaults */
  Config cfg;
  config_init_defaults(&cfg);

  /* Parse command-line arguments */
  config_parse_args(&cfg, argc, argv);

  /* Print configuration summary */
  config_print(&cfg);

  /* =========================================================================
   * STEP 1: Load and preprocess signal
   * =========================================================================
   */
  size_t sig_len;
  float final_sps;
  double pwr_raw_w = 0.0, pwr_post_w = 0.0;
  cplxf* sig =
  load_and_process(&cfg, &sig_len, &final_sps, &pwr_raw_w, &pwr_post_w);
  if (!sig) return 1;

  if (!sig) {
    return 1;
  }

  printf("\n=== POWER (measured from voltage samples) ===\n");
  printf("Ptot_raw  : %.6e W (%.2f dBm)\n", pwr_raw_w, watt_to_dbm(pwr_raw_w));
  printf("Ptot_post : %.6e W (%.2f dBm)\n", pwr_post_w,
         watt_to_dbm(pwr_post_w));
  /* =========================================================================
   * STEP 2: Demodulation (Costas + Timing Recovery)
   * =========================================================================
   */
  cplxf* costas = NULL;
  cplxf* syms_qpsk = NULL;
  float* syms_bpsk = NULL;
  float* freq_log = NULL;
  float* sps_log = NULL;
  size_t nsyms = 0, nlog = 0;

#if ENABLE_AUTO_TUNE
  /* =========================================================================
   * AUTO-TUNING: Grid search for optimal loop parameters
   * =========================================================================
   */
  printf("\n=== AUTO-TUNING (%s) ===\n",
         cfg.modulation == MOD_BPSK ? "BPSK" : "OQPSK");

  float costas_alpha_range[] = {0.01f, 0.03f, 0.05f, 0.07f, 0.1f};
  float costas_beta_range[] = {0.00005f, 0.0001f, 0.00015f, 0.0002f, 0.0003f};
  float timing_alpha_range[] = {0.01f, 0.03f, 0.05f, 0.07f, 0.1f};
  float timing_beta_range[] = {0.001f, 0.003f, 0.005f, 0.007f, 0.01f};

  int n_ca = sizeof(costas_alpha_range) / sizeof(float);
  int n_cb = sizeof(costas_beta_range) / sizeof(float);
  int n_ta = sizeof(timing_alpha_range) / sizeof(float);
  int n_tb = sizeof(timing_beta_range) / sizeof(float);

  float best_evm = 1000.0f;
  float best_ca = cfg.costas_alpha, best_cb = cfg.costas_beta;
  float best_ta = cfg.timing_alpha, best_tb = cfg.timing_beta;
  int total_tests = n_ca * n_cb * n_ta * n_tb;
  int test_count = 0;

  printf("Testing %d combinations...\n", total_tests);

  for (int i_ca = 0; i_ca < n_ca; i_ca++) {
    for (int i_cb = 0; i_cb < n_cb; i_cb++) {
      for (int i_ta = 0; i_ta < n_ta; i_ta++) {
        for (int i_tb = 0; i_tb < n_tb; i_tb++) {
          test_count++;
          Config test_cfg = cfg;
          test_cfg.costas_alpha = costas_alpha_range[i_ca];
          test_cfg.costas_beta = costas_beta_range[i_cb];
          test_cfg.timing_alpha = timing_alpha_range[i_ta];
          test_cfg.timing_beta = timing_beta_range[i_tb];

          float evm = 1000.0f;

          if (cfg.modulation == MOD_BPSK) {
            cplxf* tc;
            float *ts, *tfl, *tsl;
            size_t tns, tnl;
            run_loops_bpsk(sig, sig_len, final_sps, &test_cfg, &tc, &ts, &tns,
                           &tfl, &tsl, &tnl, 1);
            if (tns > (size_t)cfg.evm_skip_syms + 1000) {
              size_t start = cfg.evm_skip_syms;
              if (cfg.evm_last_syms > 0 &&
                  (size_t)cfg.evm_last_syms < (tns - start))
                start = tns - cfg.evm_last_syms;
              evm = evm_decision_directed_bpsk(ts + start, tns - start);
            }
            free(tc);
            free(ts);
            if (tfl) free(tfl);
            if (tsl) free(tsl);
          } else {
            cplxf *tc, *ts;
            float *tfl, *tsl;
            size_t tns, tnl;
            run_loops(sig, sig_len, final_sps, &test_cfg, &tc, &ts, &tns, &tfl,
                      &tsl, &tnl, 1);
            if (tns > (size_t)cfg.evm_skip_syms + 1000) {
              size_t start = cfg.evm_skip_syms;
              if (cfg.evm_last_syms > 0 &&
                  (size_t)cfg.evm_last_syms < (tns - start))
                start = tns - cfg.evm_last_syms;
              evm = evm_decision_directed_qpsk(ts + start, tns - start);
            }
            free(tc);
            free(ts);
            if (tfl) free(tfl);
            if (tsl) free(tsl);
          }

          if (evm < best_evm) {
            best_evm = evm;
            best_ca = test_cfg.costas_alpha;
            best_cb = test_cfg.costas_beta;
            best_ta = test_cfg.timing_alpha;
            best_tb = test_cfg.timing_beta;
            printf(
                "[%3d/%3d] NEW BEST: EVM=%.4f%% CA=%.3f CB=%.5f TA=%.3f "
                "TB=%.3f\n",
                test_count, total_tests, evm * 100.0f, best_ca, best_cb,
                best_ta, best_tb);
          }

          if ((test_count % 10) == 0) {
            printf("[%d/%d] still running... current best EVM=%.4f%%\n",
                   test_count, total_tests, best_evm * 100.0f);
            fflush(stdout);
          }
        }
      }
    }
  }

  /* Apply best parameters */
  cfg.costas_alpha = best_ca;
  cfg.costas_beta = best_cb;
  cfg.timing_alpha = best_ta;
  cfg.timing_beta = best_tb;
  printf("\nBest EVM: %.4f%% (%.2f dB)\n", best_evm * 100.0f,
         20.0f * log10f(best_evm));
#endif /* ENABLE_AUTO_TUNE */

  /* Run final demodulation */
  if (cfg.modulation == MOD_BPSK) {
    run_loops_bpsk(sig, sig_len, final_sps, &cfg, &costas, &syms_bpsk, &nsyms,
                   &freq_log, &sps_log, &nlog, 0);
  } else {
    run_loops(sig, sig_len, final_sps, &cfg, &costas, &syms_qpsk, &nsyms,
              &freq_log, &sps_log, &nlog, 0);
  }

  printf("---bruh---");
  printf("\n--- DEMODULATED ---\n");
  printf("Symbols: %zu\n", nsyms);

  /* =========================================================================
   * STEP 3: Symbol to Bit Conversion
   * =========================================================================
   */
  size_t total_bits;
  unsigned char* demod_bits;

  if (cfg.modulation == MOD_BPSK) {
    total_bits = nsyms;
    printf("Bits: %zu (1 bit/symbol)\n", total_bits);
    demod_bits = (unsigned char*)malloc(total_bits);
    for (size_t i = 0; i < nsyms; i++) {
      demod_bits[i] = syms_bpsk[i] >= 0 ? 1 : 0;
    }
  } else {
    total_bits = nsyms * 2;
    printf("Bits: %zu (2 bits/symbol)\n", total_bits);
    demod_bits = (unsigned char*)malloc(total_bits);
    for (size_t i = 0; i < nsyms; i++) {
      demod_bits[i * 2] = syms_qpsk[i].re >= 0 ? 1 : 0;
      demod_bits[i * 2 + 1] = syms_qpsk[i].im >= 0 ? 1 : 0;
    }
  }

  /* =========================================================================
   * OPTIONAL: UDP Streaming (infinite loop)
   * =========================================================================
   */
#if ENABLE_UDP_SENDER
  printf("\n--- ENTERING INFINITE UDP SEND LOOP ---\n");
  printf("   Press Ctrl+C to stop\n");

  size_t loop_count = 0;
  while (1) {
    loop_count++;
    printf("\n[Loop %zu] Sending demodulated bits...\n", loop_count);
    udp_send_unpacked_bits(demod_bits, total_bits, UDP_HOST, UDP_PORT,
                           UDP_CHUNK_SIZE);
  }
#endif

  /* =========================================================================
   * STEP 4: Blind Processing Chain
   * =========================================================================
   */
  printf("\n--- BLIND PROCESSING ---\n");
  unsigned char* processed_bits = (unsigned char*)malloc(total_bits);
  int processed_len =
      process_blind(demod_bits, processed_bits, (int)total_bits);

  if (processed_len > 0) {
    printf("Output: %d bits\n", processed_len);

    /* Save output bits to file */
    FILE* out = fopen("output_bits.txt", "w");
    if (out) {
      for (int i = 0; i < processed_len; i++) {
        fputc(processed_bits[i] ? '1' : '0', out);
      }
      fclose(out);
      printf("Saved to output_bits.txt\n");
    }

    /* =====================================================================
     * STEP 5: Frame Sync & Reed-Solomon Decode (CCSDS)
     * =====================================================================
     */
    printf("\n--- FRAME SYNC & RS DECODE (0x1ACFFC1D) ---\n");

    /* CCSDS sync word: 0x1ACFFC1D in bit form (MSB first) */
    const unsigned char sync_pattern[32] = {
        0, 0, 0, 1, 1, 0, 1, 0, /* 0x1A */
        1, 1, 0, 0, 1, 1, 1, 1, /* 0xCF */
        1, 1, 1, 1, 1, 1, 0, 0, /* 0xFC */
        0, 0, 0, 1, 1, 1, 0, 1  /* 0x1D */
    };

    static int last_error_counter_rs = 0;
    int tm_ok_count = 0;
    int tm_bad_count = 0;
    int frame_count = 0;

    /* Search for sync and process frames */
    int offset = 0;
    while (offset <= processed_len - FRAME_SIZE_BITS) {
      /* Find sync word */
      int sync_found = -1;
      for (int i = offset; i <= processed_len - FRAME_SIZE_BITS; i++) {
        int match = 1;
        for (int j = 0; j < SYNC_BITS && match; j++) {
          if (processed_bits[i + j] != sync_pattern[j]) {
            match = 0;
          }
        }
        if (match) {
          sync_found = i;
          break;
        }
      }

      if (sync_found < 0) {
        printf("No more sync words found.\n");
        break;
      }

      frame_count++;
      printf("Frame %d: Sync at bit %d", frame_count, sync_found);

      /* Check if we have enough bits for a full frame */
      if (sync_found + FRAME_SIZE_BITS > processed_len) {
        printf(" - Incomplete frame, skipping.\n");
        break;
      }

      /* Pack bits to bytes (1279 bytes = 10232 bits) */
      unsigned char frame_bytes[FRAME_SIZE_BYTES];
      bitsToBytes(&processed_bits[sync_found], frame_bytes, FRAME_SIZE_BITS);

      /* Create working buffer for RS decode */
      unsigned char tempBuffer[FRAME_SIZE_BYTES];
      memcpy(tempBuffer, frame_bytes, FRAME_SIZE_BYTES);

      /* Descramble (skip first 4 bytes - sync word) */
#ifdef __cplusplus
      Scrambler s;
      s.Scramble(&tempBuffer[4]);
#endif

      /* Reed-Solomon decode */
      int msgSize = FRAME_SIZE_BYTES;
      int error_counter_rs = ccsds_decode_rs(tempBuffer, msgSize);

      if (error_counter_rs == last_error_counter_rs) {
        /* RS decode successful */
        msgSize -= 4; /* Remove sync word (4 bytes) */

        unsigned char TM_Frame_buffer[FRAME_SIZE_BYTES];
        memmove(TM_Frame_buffer, tempBuffer + 4, msgSize);

        msgSize -=
            160; /* Remove RS parity bytes (160 bytes for interleave=5) */

        printf(" - RS OK, TM size = %d bytes\n", msgSize);

        /* Print the entire TM frame in hex */
        printf("================== TM FRAME %d ==================\n",
               tm_ok_count + 1);
        for (int i = 0; i < msgSize; i++) {
          printf("%02X ", TM_Frame_buffer[i]);
          if ((i + 1) % 32 == 0) {
            printf("\n");
          }
        }
        if (msgSize % 32 != 0) {
          printf("\n");
        }
        printf("================================================\n\n");

        tm_ok_count++;
      } else {
        last_error_counter_rs = error_counter_rs;
        printf(" - RS FAILED (errors=%d)\n", error_counter_rs);
        tm_bad_count++;
      }

      /* Move to next potential frame */
      offset = sync_found + FRAME_SIZE_BITS;
    }

    printf("\n--- FRAME PROCESSING SUMMARY ---\n");
    printf("Frames found:  %d\n", frame_count);
    printf("TM OK:         %d\n", tm_ok_count);
    printf("TM BAD:        %d\n", tm_bad_count);
    printf("Success rate:  %.1f%%\n",
           frame_count > 0 ? (100.0f * tm_ok_count / frame_count) : 0.0f);
  }

  /* =========================================================================
   * STEP 6: EVM Calculation
   * =========================================================================
   */
  if (nsyms > (size_t)cfg.evm_skip_syms + 1000) {
    size_t start = cfg.evm_skip_syms;
    if (cfg.evm_last_syms > 0 && (size_t)cfg.evm_last_syms < (nsyms - start)) {
      start = nsyms - cfg.evm_last_syms;
    }
    size_t evm_n = nsyms - start;



       float evm = cfg.modulation == MOD_BPSK
                    ? evm_decision_directed_bpsk(syms_bpsk + start, evm_n)
                    : evm_decision_directed_qpsk(syms_qpsk + start, evm_n);

    printf("\n=== EVM (%s) ===\n",
           cfg.modulation == MOD_BPSK ? "BPSK" : "OQPSK");
    printf("EVM: %.4f%% (%.2f dB)\n", evm * 100.0f,
           20.0f * log10f(evm + 1e-30f));
    printf("SNR(rough): ~%.2f dB\n", -20.0f * log10f(evm + 1e-30f));

    // ================= ENERGY / NOISE REPORT =================
    {
      const double bits_per_sym = (cfg.modulation == MOD_BPSK) ? 1.0 : 2.0;

      // Bu kod tabanında cfg.rb pratikte "bit rate" gibi kullanılıyor (RRC'de
      // rb/2 yapılmış).
      const double Rb = (double)cfg.rb;     // bit/s
      const double Rs = Rb / bits_per_sym;  // sym/s

      const double alpha = (cfg.rrc_enable ? (double)cfg.rrc_alpha : 0.0);
      const double Bocc =
          Rs * (1.0 + alpha);  // Hz (two-sided, sizin eski çıktınızla uyumlu)

      // EVM -> Es/N0 varsayımı (AWGN + iyi lock + iyi eşitleme varsayımı)
      const double evm2 = (double)evm * (double)evm + 1e-30;
      const double EsN0 = 1.0 / evm2;
      const double EbN0 = EsN0 / bits_per_sym;

      // Eb/N0 ile in-band SNR ilişkisi: SNR = (Eb/N0) * (Rb / Bocc)
      const double SNR_lin = EbN0 * (Rb / (Bocc + 1e-30));

      // Ölçtüğünüz toplam güçten (post) Psig ve Pn'i ayır (yaklaşık)
      const double Ptot = pwr_post_w;
      const double Psig = Ptot * (SNR_lin / (1.0 + SNR_lin));
      const double Pn_inband = Ptot * (1.0 / (1.0 + SNR_lin));

      // Enerjiler
      const double Eb = Psig / (Rb + 1e-30);  // J/bit
      const double Es = Psig / (Rs + 1e-30);  // J/sym

      // Noise spectral density
      const double N0 = Pn_inband / (Bocc + 1e-30);  // W/Hz (== J)

      printf("\n=== ENERGY / NOISE (est.) ===\n");
      printf("Rb            : %.3e bit/s\n", Rb);
      printf("Rs            : %.3e sym/s\n", Rs);
      printf("Bocc (est)    : %.3e Hz  (alpha=%.2f)\n", Bocc, alpha);

      printf("Psig_post (est): %.6e W (%.2f dBm)\n", Psig, watt_to_dbm(Psig));
      printf("Pn_inband (est): %.6e W (%.2f dBm)\n", Pn_inband,
             watt_to_dbm(Pn_inband));
      printf("N0 (est)      : %.6e W/Hz (%.2f dBm/Hz)\n", N0,
             w_per_hz_to_dbm_per_hz(N0));

      printf("Eb            : %.6e J/bit\n", Eb);
      printf("Es            : %.6e J/sym\n", Es);

      printf("Es/N0         : %.2f dB\n", 10.0 * log10(EsN0 + 1e-30));
      printf("Eb/N0         : %.2f dB\n", 10.0 * log10(EbN0 + 1e-30));
      printf("SNR_inband    : %.2f dB\n", 10.0 * log10(SNR_lin + 1e-30));
    }

  }

  /* =========================================================================
   * CLEANUP
   * =========================================================================
   */
  free(demod_bits);
  free(processed_bits);
  free(sig);
  free(costas);
  if (syms_qpsk) free(syms_qpsk);
  if (syms_bpsk) free(syms_bpsk);
  if (freq_log) free(freq_log);
  if (sps_log) free(sps_log);

  return 0;
}