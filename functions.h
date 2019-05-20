#define if_up   0x01
#define if_down 0x00

struct options {
    char *iface;
    char *set;
    int check;  
};

/* holds interface info */
typedef struct ic_info {
    char *ip;
    char *netmask;
    char *name;
    char *hwaddr;
    char *bcast;
    int mtu;
    int index;
} ic_info_t;

typedef enum { IPV4, IPV6, BCAST, MAC, MASK } ic_adrtype_t;

static void show_usage( void );
static void free_ic_info( ic_info_t *ic_info );
static void print_iface_info( ic_info_t ic_info );
static int is_running( const char *ifname );
static int iface_info( int *sock, char *ifname, ic_info_t *ic_info );
static int ic_get_mtu( int *sock, struct ifreq *i_req );
static int ic_get_index( int *sock, struct ifreq *i_req );
static int set_if_run_flags( int *sock, char *ifname, short stat );
static int ic_get_run_flags( int *sock, char *ifname, short *flags );
static unsigned char * ic_hwaddr_frmt( char *hwaddr );
static char * ic_get_addr( int *sock, struct ifreq *i_req, ic_adrtype_t adrtype );
static char * alloc_chrmem( size_t size );
