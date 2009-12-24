#define INDEX_CMD_REQUEST 0
#define INDEX_CMD_VALUE 1
#define INDEX_PAYLOAD_SIZE 2

unsigned int slogic_firm_cmds[] ={
#include "firm_cmds.inc"
};
unsigned char slogic_firm_data[]= {
#include "firm_data.inc"
};

int slogic_firm_cmds_size()
{
    return sizeof(slogic_firm_cmds);
}
