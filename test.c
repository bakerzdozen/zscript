// Online C compiler to run C program online
#include <stdio.h>

#define BYTE    unsigned char
#define WORD    unsigned short
#define DWORD   unsigned long
#define SBYTE   signed char
#define SWORD   signed short
#define SDWORD  signed long

#define BOOL    BYTE
#define TRUE    1
#define FALSE   0

#define MAX_STARTUP_CMDS                10
#define MAX_ADDITIONAL_REGISTERS        10

typedef enum _sdi_meas_type {
    SDI_MEAS_M = 0,
    SDI_MEAS_C,
    SDI_MEAS_R,
    SDI_MEAS_X
} sdi12MeasType_t;

typedef struct _sdi12_set_cmd {
    BYTE row;
    BYTE data[85];
    BYTE data_len;
    BYTE read_indx;
    sdi12MeasType_t type;
} sdi12Measurement_t;

typedef enum _vwc_mod_err
{
    VWC_OK = 0,
    VWC_ERR_INVALID_ARG,
    VWC_ERR_TIMEOUT,
    VWC_ERR_FULL,
    VWC_ERR_BOOT_FAILURE,
    VWC_ERR_INVALID_RESPONSE
} vwc_err_t;

typedef struct _vwc_cmd {
    BYTE reg_addr;
    WORD val;
} vwcCommand_t;

typedef struct _vwc_config {
    BYTE no_startup_cmds;                                   // 1 byte
    BYTE no_additional_registers;                           // 1 byte
    BYTE additional_registers[MAX_ADDITIONAL_REGISTERS];    // 20 bytes
    WORD measurement_delay_ms;                              // 2 bytes
                                                            // TOTAL: 24 bytes
} vwcConfig_t;

static vwcConfig_t startup_config;
static vwcCommand_t startup_cmds[MAX_STARTUP_CMDS];

typedef enum _sdi12_data_type
{
    COMM_DATA_UINT8 = 0x1,
    COMM_DATA_UINT16 = 0x2,
    COMM_DATA_UINT32 = 0x3,    // Don't support >32 bit integers
    COMM_DATA_INT8 = 0x5,
    COMM_DATA_INT16 = 0x6,
    COMM_DATA_INT32 = 0x7,
    COMM_DATA_HEX = 0xF,
    COMM_DATA_FLOAT = 0x1F,
    COMM_DATA_STRING = 0x3F,
    COMM_DATA_BYTES = 0x7F,
    COMM_DATA_INVALID = 0xFF,
} sdi12DataType_t;

typedef union _sdi12_data {
    SDWORD i;
    float f;
    const BYTE* s;
} sdi12Data_t;

typedef struct _sdi12_data_point {
    sdi12DataType_t dtype;
    sdi12Data_t data;
    BYTE size;
} sdi12DataPoint_t;

static sdi12DataPoint_t data_points[6];

BYTE char_to_num(BYTE ch)
{
    return ch - '0';
}

static BYTE num_digits(SDWORD num)
{
    BYTE size = 2;
    
    if (num < 0) {
        num *= -1;
    }
    for (; size <= 7; size++) {
        if (num < 10) {
            break;
        }
        num /= 10;
    }
    return size;
}

BYTE sdi12_strlen(const BYTE *str) {
    
    BYTE len = 0;
    
    while (*str != '\0') {
        len++;
        str++;
    }
    
    return len;
}

BOOL sdi12_isdigit(BYTE val) {
    val -= '0';
    return val <= 9;
}

BOOL sdi12_ishex(BYTE val) {
    return sdi12_isdigit(val) || ((BYTE)(val - 'A') <= 5);
}

SDWORD sdi12_atoi(const BYTE * ptr) {
    
    SBYTE result = 1;
    SDWORD res = 0;

    if (*ptr == '-') {
        result = -1;
        ptr++;
    }

    if (*ptr == '+') {
        ptr++;
    }

    for (BYTE loop = 0; sdi12_isdigit(ptr[loop]); loop++) {
        res = (res * 10) + char_to_num(ptr[loop]);
    }

    return (result * res);
}

BYTE sdi12_itostr(SDWORD data, BYTE *ch, BOOL sgn, BYTE num_digits)
{
    BYTE *p = ch;
    if (data < 0) {
        *p++ = '-';
        data = -data;
    } else if (sgn) {
        *p++ = '+';
    }

    // Convert to string in reverse
    BYTE buf[10], n = 0;
    do {
        buf[n++] = (BYTE)('0' + (data % 10));
        data /= 10;
    } while (data);

    // Zero-padding
    while (n < num_digits) buf[n++] = '0';

    // Reverse copy
    while (n--) *p++ = buf[n];

    *p = '\0';

    return (BYTE)(p - ch);
}

BYTE* sdi12_ftostr(float data, BYTE *ch)
{
    BYTE size = 1;

    // Integer & decimal
    SDWORD int_part = (SDWORD)data;
    float diff = (data - (float)int_part);
    if (diff < 0) {
        diff *= -1;
    }
    diff *= 100.0f + 0.5f; // round 2dp
    size += sdi12_itostr(int_part, ch, TRUE, 0);

    SBYTE dec_digits = 2;
    if (size > 6) {
        return ch;
    }
    ch[size++] = '.';
    sdi12_itostr((DWORD)diff, &ch[size], FALSE, dec_digits);

    return ch;
}

void print_startup_cmds()
{
    printf("Number of commands: %u\n", startup_config.no_startup_cmds);
    for (int i = 0; i < startup_config.no_startup_cmds; i++) {
        printf("Reg: %02u : %u\n", startup_cmds[i].reg_addr, startup_cmds[i].val);
    }
}

void print_data_type(sdi12DataType_t dtype) {
    switch (dtype) {
        case COMM_DATA_UINT8:
            printf("COMM_DATA_UINT8\n");
            break;
        case COMM_DATA_UINT16:
            printf("COMM_DATA_UINT16\n");
            break;
        case COMM_DATA_UINT32:
            printf("COMM_DATA_UINT32\n");
            break;
        case COMM_DATA_INT8:
            printf("COMM_DATA_INT8\n");
            break;
        case COMM_DATA_INT16:
            printf("COMM_DATA_INT16\n");
            break;
        case COMM_DATA_INT32:
            printf("COMM_DATA_INT32\n");
            break;
        case COMM_DATA_HEX:
            printf("COMM_DATA_HEX\n");
            break;
        case COMM_DATA_FLOAT:
            printf("COMM_DATA_FLOAT\n");
            break;
        case COMM_DATA_STRING:
            printf("COMM_DATA_STRING\n");
            break;
        case COMM_DATA_BYTES:
            printf("COMM_DATA_BYTES\n");
            break;
        default: 
            printf("COMM_DATA_INVALID\n");
            break;
        }
}

sdi12DataType_t get_data_type(const BYTE* data) 
{
    if (data == NULL) {
        return COMM_DATA_INVALID;
    }
    sdi12DataType_t dtype = 0x00;
    BOOL is_signed = FALSE;

    const BYTE* p = data;
    
    if (*p == '-') {
        p++;
        is_signed = TRUE;
    } else if (*p == '+') {
        p++;
    }

    BYTE len = sdi12_strlen(p);
    
    while (*p) {
        
        if (sdi12_isdigit(*p)) {
            dtype |= COMM_DATA_UINT8;
        } else if (sdi12_ishex(*p)) {
            dtype |= COMM_DATA_HEX;
        } else if (*p == '.') {
            if (dtype == COMM_DATA_INVALID) {
                // Check for additional elements
                if (sdi12_isdigit(*(p + 1))) {
                    dtype |= COMM_DATA_FLOAT;
                } else {
                    dtype |= COMM_DATA_STRING;
                }
            } else if (dtype <= COMM_DATA_INT32) {
                dtype |= COMM_DATA_FLOAT;
            } else {
                dtype |= COMM_DATA_STRING;
            }
        } else if ((*p == '+') || (*p == '-')) {
            // Next SDI12 measurement
            break;
        } else if (*p >= ' ') {
            // Not a printable character
            dtype |= COMM_DATA_STRING;
        } else {
            return COMM_DATA_INVALID;
        }
        p++;
    }

    if (dtype == 0x00) {
        dtype = COMM_DATA_INVALID;
    } else if ((dtype == COMM_DATA_HEX) && (len & 0x1)) {
        return COMM_DATA_INVALID;
    } else if (is_signed && (dtype > COMM_DATA_FLOAT)) {
        return COMM_DATA_INVALID;
    } else if (dtype && (dtype <= COMM_DATA_INT32)) {
        if (is_signed) {
            dtype |= 0x4;
        }
        if (len > 9) {
            // Don't support 1,000,000,000 or above
            return COMM_DATA_INVALID;
        }
        SDWORD num = sdi12_atoi(data);
        if (num < 0xFF) {
            return dtype;
        } 
        dtype++;
        if (num < 0xFFFF) {
            return dtype;
        }
        dtype++;
    }

    return dtype;
}

void print_data(sdi12DataPoint_t p)
{
    switch (p.dtype) {
        case COMM_DATA_UINT8:
            printf("Data: %u (uint8) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_UINT16:
            printf("Data: %u (uint16) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_UINT32:
            printf("Data: %u (uint32) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_INT8:
            printf("Data: %u (int8) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_INT16:
            printf("Data: %u (int16) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_INT32:
            printf("Data: %u (int32) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_FLOAT:
            printf("Data: %f (float) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_HEX:
            printf("Data: 0x%X (hex) - size %u\n", p.data, p.size);
            break;
        case COMM_DATA_STRING:
            printf("Data: %s (string) - size %u\n", p.data, p.size);
            break;
        default:
            printf("Data: INVALID (INVALID) - size %u\n", p.size);
            
    }
}

BYTE get_startup_reg_pos(BYTE reg_addr) {
    for (int i = 0; i < startup_config.no_startup_cmds; i++) {
        if (startup_cmds[i].reg_addr == reg_addr) {
            return i;
        }
    }
    return startup_config.no_startup_cmds;
}

vwc_err_t remove_startup_cmd(BYTE addr)
{
    BYTE cmd_no = get_startup_reg_pos(addr);
    
    if (cmd_no >= startup_config.no_startup_cmds) {
        return VWC_ERR_INVALID_ARG;
    }
    
    startup_config.no_startup_cmds--;
    
    for (BYTE i = cmd_no; i < startup_config.no_startup_cmds; i++) {
        startup_cmds[i].reg_addr = startup_cmds[i + 1].reg_addr;
        startup_cmds[i].val = startup_cmds[i + 1].val;
    }
    
    return VWC_OK;
}

vwc_err_t add_startup_cmd(BYTE reg_addr, WORD val)
{
    // Cannot change address or an invalid register
    if ((reg_addr == 0) || (reg_addr >= 50)) {
        return VWC_ERR_INVALID_ARG;
    }
    
    BYTE pos = get_startup_reg_pos(reg_addr);
    
    if (pos >= MAX_STARTUP_CMDS) {
        return VWC_ERR_FULL;
    }

    if (pos == startup_config.no_startup_cmds) {
        startup_config.no_startup_cmds++;
    }

    startup_cmds[pos].reg_addr = reg_addr;
    startup_cmds[pos].val = val;
    
    return VWC_OK;    
}

BYTE get_additional_measurement_pos(BYTE addr)
{
    for (int i = 0; i < startup_config.no_additional_registers; i++) {
        if (startup_config.additional_registers[i] == addr) {
            return i;
        }
    }
    return startup_config.no_additional_registers;
}

// TODO: Change both removes to take the register not the position
vwc_err_t remove_additional_measurement(BYTE addr)
{
    BYTE pos = get_additional_measurement_pos(addr);
    
    if (pos >= startup_config.no_additional_registers) {
        return VWC_ERR_INVALID_ARG;
    }

    startup_config.no_additional_registers--;

    for (BYTE i = pos; i < startup_config.no_additional_registers; i++) {
        startup_config.additional_registers[i] = startup_config.additional_registers[i + 1];
    }
    
    return VWC_OK;
}

vwc_err_t add_additional_measurement(BYTE addr)
{
    if (addr > 50) {
        return VWC_ERR_INVALID_ARG;
    }
    BYTE pos = get_additional_measurement_pos(addr);
 
    if (startup_config.no_additional_registers >= MAX_ADDITIONAL_REGISTERS) {
        return VWC_ERR_FULL;
    }
    
    // We place the register into 'pos' even if 'pos' already contains that value
    startup_config.additional_registers[pos] = addr;
    // If it is new
    if (pos == startup_config.no_additional_registers) {
        startup_config.no_additional_registers++;
    }

    return VWC_OK;
}

BYTE* sdi12_set_get_next(sdi12Measurement_t* cmd)
{
    if (cmd == NULL) {
        return NULL;
    }
    static BYTE buff[85];
    BYTE windx = 0;

    do {
        switch (cmd->data[cmd->read_indx]) {
            case '\0':
                if (windx) {
                    buff[windx] = '\0';
                    return buff;
                }
                return NULL;
            case '+':
            case '-':
                if (windx) {
                    buff[windx] = '\0';
                    return buff;
                }
            default:
                buff[windx++] = cmd->data[cmd->read_indx];
                break;
        }
    } while (++cmd->read_indx < cmd->data_len);

    if (windx) {
        buff[windx] = '\0';
        return buff;
    }
    return NULL;
}

BYTE uart_read_byte()
{
    const BYTE data[] = "TESTING123 this should not match but BOOT OK should";
    static WORD i = 0;

    if (i == sizeof(data)) {
        printf("HERE\n");
        i = 0;
    }

    return data[i++];
}

vwc_err_t wait_for_match(const BYTE* match, WORD timeout_ms)
{
    BYTE ret_byte; 
    BYTE str_indx = 0;
    
    if ((match == NULL) || (match[0] == '\0')) {
        return VWC_ERR_INVALID_ARG;
    }
    
    for (WORD t = 0; t < timeout_ms; t += 1) {
        ret_byte = uart_read_byte();
        if (ret_byte == match[str_indx]) {
            if (match[++str_indx] == '\0') {
                return VWC_OK;
            }
        } else {
            str_indx = 0;
        }
    }
    
    return VWC_ERR_TIMEOUT;
}

int main() {
    // Write C code here
    BYTE str[258] = "+1+65000+7+420+5+800";
    BYTE data[100] = "#ABCD";
    float val = 1234567.5432;
    // printf("Input: %4f | Output: %4s\n", val, sdi12_ftostr(val, data));
    // printf("Digits: %u\n", num_digits((SDWORD)val));

const char *sdi12_test_strings[] =
{
    // --- INT (signed integer) ---
    "-123",            // COMM_DATA_INT: valid negative integer
    "-00042",          // COMM_DATA_INT: leading zeros
    "-9",              // COMM_DATA_INT: single-digit negative
    "-12.0",           // COMM_DATA_FLOAT: has a decimal -> not int
    "-12A",            // COMM_DATA_STRING: invalid char for int

    // --- UINT (unsigned integer) ---
    "+0",              // COMM_DATA_UINT: valid zero
    "+12345",          // COMM_DATA_UINT: valid unsigned integer
    "+987654321",      // COMM_DATA_UINT: valid, large number
    "+0042",           // COMM_DATA_UINT: valid, leading zeros
    "+42.1",           // COMM_DATA_FLOAT: decimal -> not uint
    "+42A",            // COMM_DATA_STRING: invalid char for uint

    // --- HEX (unsigned hexadecimal) ---
    "+AABB0055",       // COMM_DATA_HEX: valid hex
    "+FF",             // COMM_DATA_HEX: valid hex
    "+0A0B0C",         // COMM_DATA_HEX: valid hex with leading zero
    "+123ABC",         // COMM_DATA_HEX: valid hex (mixed digits/letters)
    "+123G",           // COMM_DATA_STRING: invalid hex (G invalid)
    "+12 34",          // COMM_DATA_STRING: invalid, space not allowed

    // --- FLOAT (signed floating-point) ---
    "+1.0",            // COMM_DATA_FLOAT: valid float
    "-123.456",        // COMM_DATA_FLOAT: valid negative float
    "+0.0001",         // COMM_DATA_FLOAT: small float
    "+42.",            // COMM_DATA_FLOAT: trailing decimal allowed
    "+.42",            // COMM_DATA_FLOAT: leading decimal allowed
    "+42.42.42",       // COMM_DATA_STRING: multiple decimals invalid
    "+4e10",           // COMM_DATA_STRING: exponential notation not SDI-12 compliant

    // --- STRING (explicitly non-numeric) ---
    "+HELLO",          // COMM_DATA_STRING: alphabetic
    "+123ABCDEFZ",     // COMM_DATA_STRING: contains invalid hex char (Z)
    "+4.2V",           // COMM_DATA_STRING: numeric + suffix
    "+DATA123",        // COMM_DATA_STRING: letters first
    "+ABCD123!",       // COMM_DATA_STRING: special char
    "+",               // COMM_DATA_INVALID or STRING: depends on your spec (empty data)
    "+   ",            // COMM_DATA_INVALID: whitespace

    // --- INVALID / edge cases ---
    "+1000000000",     // COMM_DATA_INVALID: missing '+' or '-'
    "",                // COMM_DATA_INVALID: empty string
    "- ",              // COMM_DATA_INVALID: sign but no digits
    "+--42",           // COMM_DATA_INVALID: double sign
    "+12+34",          // COMM_DATA_STRING: extra sign in middle
};

    // for (int i = 0; i < (sizeof(sdi12_test_strings) / sizeof(char*)); i++) {
    //     printf("%15s : ", sdi12_test_strings[i]);
    //     print_data_type(get_data_type(sdi12_test_strings[i]));
    // }

    print_data_type(get_data_type("0203000000018439"));
    
    // sdi12DataPoint_t dpoint = {
    //     .data = 34,
    //     .dtype = COMM_DATA_INT8,
    //     .size = 3
    // };

    // print_data(dpoint);
    
    // data_points[3] = dpoint;

    // print_data(data_points[3]);

    // WORD delay_s = (500 * (5 + 1)) / 1000;

    // BYTE time = delay_s + (5000 / 1000) + (1 / 5);

    // printf("Delay: %u\n", delay_s);
    // printf("Time: %u\n", time + 5);

    print_startup_cmds();

    sdi12Measurement_t c_raw = {
        .data = "+5+800",
        .read_indx = 0,
        .row = 0,
        .type = SDI_MEAS_X
    };

    c_raw.data_len = sdi12_strlen(c_raw.data);

    sdi12Measurement_t* c = &c_raw;

    BYTE* buff = sdi12_set_get_next(c);

    vwc_err_t res = VWC_OK;

    startup_cmds[0].reg_addr = 5;
    startup_cmds[0].val = 0;
    
    startup_cmds[1].reg_addr = 10;
    startup_cmds[1].val = 109;
    
    startup_cmds[2].reg_addr = 19;
    startup_cmds[2].val = 3;
    
    startup_cmds[3].reg_addr = 20;
    startup_cmds[3].val = 5;

    startup_config.no_startup_cmds = 4;

    // do {
    //     if (get_data_type(buff) != COMM_DATA_UINT8) {
    //         return VWC_ERR_INVALID_ARG;
    //     }
        
    //     BYTE addr = sdi12_atoi(buff);

    //     buff = sdi12_set_get_next(c);
        
    //     if (get_data_type(buff) > COMM_DATA_UINT16) {
    //         return VWC_ERR_INVALID_ARG;
    //     }

    //     res = add_startup_cmd(addr, sdi12_atoi(buff));

    //     if (res != VWC_OK) {
    //         return res;
    //     }
        
    //     buff = sdi12_set_get_next(c);
    // } while (buff);

    // print_startup_cmds();

    res = wait_for_match("BOOT OK", 9999);

    if (res == VWC_OK) {
        printf("OK\n");
    } else {
        printf("ERR_0x%X\n", res);
    }

    return 0;
}