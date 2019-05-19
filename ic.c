#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

struct options {
    char *iface;
    char *set;
    int check;  
};

/* hold interface info */
typedef struct ic_info {
    char *ip;
    char *netmask;
    int mtu;
    char *name;
    char *hwaddr;
} ic_info_t;

static void show_usage( void );
static int is_running( const char *ifname );
static int iface_info( char *ifname, struct ifreq *if_req );
static void print_iface_info( struct ifreq *if_req );
static char * alloc_chrmem( size_t size );

int main( int argc, char **argv ) {
    int flags, opt;
    int nsecs, tfnd;
    struct ifreq if_req, if_reqs[0x0F];
    struct options o, *opts = &o;
    nsecs = tfnd = flags = 0;
    
    opts->iface = NULL;
    opts->check = 0;
    opts->set   = NULL;

    while ( (opt = getopt( argc, argv, "i:s:c" )) != -1 ) {
        switch ( opt ) {
            case 'i':
                opts->iface = alloc_chrmem( 20 );
                strcpy( opts->iface, optarg );
                break;
            case 'c':
                opts->check = 1;
                break;
            case 's':
                if ( atoi( optarg ) > 1 || atoi( optarg ) < 0 ) {
                    show_usage();
                    return -1;
                }
                opts->set = alloc_chrmem( 2 );
                strcpy( opts->set, optarg );
                break;
            default:
                show_usage();
                return -1;
        }
    }
    
    /* cannot use the -c option in combination with -s or -i solely */
    if ( opts->iface != NULL && ((opts->check == 0 && opts->set == NULL) || 
        (opts->check && opts->set != NULL)) ) {
            show_usage();
            return -1;
    } 
    else if ( opts->iface != NULL && opts->check ) {
        if ( iface_info( opts->iface, &if_req ) == 0 ) {
            print_iface_info( &if_req );
        }
    }
    free( opts->iface );
    return 0;
}

static void show_usage( void ) {
    fprintf( stdout, "Usage: ic [OPTION]...\n" );
    fprintf( stdout, "Control and check network interfaces\n" );
    fprintf( stdout, "-i\tInterface name\n" );
    fprintf( stdout, "-s\tBring interface up or down - values -> 1(up) or 0(down)\n" );
    fprintf( stdout, "-c\tDisplay interface info\n\n" );
}

static void print_iface_info( struct ifreq *if_req ) {

}

static int iface_info( char *ifname, struct ifreq *if_req ) {
    int sock;
    struct ifreq i_req;

    if ( (sock = socket( AF_INET, SOCK_STREAM, 0x00 )) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    memcpy( i_req.ifr_name, ifname, strlen( ifname ) + 1 );
    if ( ioctl( sock, SIOCGIFFLAGS, &i_req ) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    *if_req = i_req;
    return 0;
}

static char * alloc_chrmem( size_t size ) {
    void *loc;
    if ( size <= 0 ) {
        fprintf( stderr, "alloc_chrmem() - Invalid size!\n" );
        exit( EXIT_FAILURE );
    }
    if ( (loc = malloc( size * sizeof( char ) )) != NULL ) {
        memset( loc, '0', size );
    } else {
        fprintf( stderr, "alloc_chrmem() - Couldn't allocate memory!\n" );
        exit( EXIT_FAILURE );
    }
    return (char *) loc;
}