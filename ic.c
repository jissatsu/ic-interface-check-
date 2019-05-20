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
#include "functions.h"

int main( int argc, char **argv ) {
    int opt, sock;
    short flags;
    ic_info_t ic_info;
    struct options o, *opts = &o;
    
    flags       = 0;
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
                if ( !opts->iface ) {
                    show_usage();
                    return -1;
                }
                opts->check = 1;
                break;
            case 's':
                if ( (atoi( optarg ) > 1 || atoi( optarg ) < 0) || !opts->iface ) {
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
    
    if ( (sock = socket( AF_INET, SOCK_STREAM, 0x00 )) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }

    /* cannot use the -c option in combination with -s or -i solely */
    if ( (!opts->check && !opts->set ) || 
        (opts->iface != NULL && ((opts->check == 0 && opts->set == NULL) ||
        (opts->check && opts->set != NULL))) ) {
            show_usage();
            free( opts->iface ); return -1;
    }
    
    else if ( opts->iface != NULL && opts->check ) {
        if ( iface_info( &sock, opts->iface, &ic_info ) == 0 ) {
            print_iface_info( ic_info );
            free_ic_info( &ic_info );
        }
    }
    
    else if ( opts->iface != NULL && opts->set != NULL ) {
        switch ( atoi( opts->set ) ) {
            case 0:
                set_if_run_flags( &sock, opts->iface, if_down );
                break;
            case 1:
                set_if_run_flags( &sock, opts->iface, if_up );
                break;
        }
        free( opts->iface );
        free( opts->set );
    }
    close( sock ); return 0;
}

static void show_usage( void ) {
    fprintf( stdout, "Usage: ic [OPTION]...\n" );
    fprintf( stdout, "Control and check network interfaces\n" );
    fprintf( stdout, "-i\tInterface name\n" );
    fprintf( stdout, "-s\tBring interface up or down - values -> 1(up) or 0(down)\n" );
    fprintf( stdout, "-c\tDisplay interface info\n\n" );
}

static void print_iface_info( ic_info_t ic_info ) {
    fprintf( stdout, " IF_NAME      - %s\n", ic_info.name );
    fprintf( stdout, " IF_ADDR      - %s\n", ic_info.ip );
    fprintf( stdout, " IF_HWADDR    - %s\n", ic_info.hwaddr );
    fprintf( stdout, " IF_NETMASK   - %s\n", ic_info.netmask );
    fprintf( stdout, " IF_BROADCAST - %s\n", ic_info.bcast );
    fprintf( stdout, " IF_MTU       - %d\n", ic_info.mtu );
    fprintf( stdout, " IF_INDEX     - %d\n", ic_info.index );
}

static int iface_info( int *sock, char *ifname, ic_info_t *ic_info ) {
    struct ifreq i_req;

    memcpy( i_req.ifr_name, ifname, strlen( ifname ) + 1 );
    ic_info->name    = ifname;
    ic_info->mtu     = ic_get_mtu( sock, &i_req );
    ic_info->netmask = ic_get_addr( sock, &i_req, MASK );
    ic_info->ip      = ic_get_addr( sock, &i_req, IPV4 );
    ic_info->bcast   = ic_get_addr( sock, &i_req, BCAST );
    ic_info->hwaddr  = ic_get_addr( sock, &i_req, MAC );
    ic_info->index   = ic_get_index( sock, &i_req );
    return 0;
}

/* Get the MTU ( Maximum Transfer Unit ) of the device using ifr_mtu */
static int ic_get_mtu( int *sock, struct ifreq *i_req ) {
    if ( ioctl( *sock, SIOCGIFMTU, i_req ) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    return i_req->ifr_mtu;
}

/* Retrieve the interface index of the interface into ifr_ifindex */
static int ic_get_index( int *sock, struct ifreq *i_req ) {
    if ( ioctl( *sock, SIOCGIFINDEX, i_req ) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    return i_req->ifr_ifindex;
}

/* convert the device's mac address to string */
static unsigned char * ic_hwaddr_frmt( char *hwaddr ) {
    static unsigned char mac[30];
    snprintf( mac, sizeof( mac ), "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned char) hwaddr[0], (unsigned char) hwaddr[1], (unsigned char) hwaddr[2],
        (unsigned char) hwaddr[3], (unsigned char) hwaddr[4], (unsigned char) hwaddr[5]
    );
    return mac;
}

/* check if the device is a loopback interface */
static int ic_is_loopback( int *sock, char *ifname ) {
    int r_c, i_c;
    short flags;
    struct ifreq i_req;

    sprintf( i_req.ifr_name, "%s", ifname );
    r_c = ic_get_run_flags( sock, ifname, &flags );
    i_c = ioctl( *sock, SIOCGIFFLAGS, &i_req );

    if ( i_c < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    return (i_req.ifr_flags & IFF_LOOPBACK);
}

static char * ic_get_addr( int *sock, struct ifreq *i_req, ic_adrtype_t adrtype ) {
    int r_c;
    char *addr;
    struct sockaddr *sa;

    switch ( adrtype ) {
        case MASK:
            r_c = ioctl( *sock, SIOCGIFNETMASK, i_req );
            sa  = &i_req->ifr_netmask;
            break;
        case IPV4:
            r_c = ioctl( *sock, SIOCGIFADDR, i_req );
            sa  = &i_req->ifr_addr;
            break;
        case BCAST:
            r_c = ioctl( *sock, SIOCGIFBRDADDR, i_req );
            sa  = &i_req->ifr_broadaddr;
            break;
        case MAC:
            r_c = ioctl( *sock, SIOCGIFHWADDR, i_req );
            sa  = &i_req->ifr_hwaddr;
            break;
    }
    if ( r_c < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return NULL;
    }
    addr = alloc_chrmem( 30 );
    if ( addr ) {
        if ( adrtype != MAC ) {
            struct sockaddr_in *sin = (struct sockaddr_in *) sa;
            sprintf( addr, "%s", inet_ntoa( sin->sin_addr ) );
        } else {
            addr = ic_hwaddr_frmt( sa->sa_data );
        }
    }
    return addr;
}

static int ic_get_run_flags( int *sock, char *ifname, short *flags ) {
    struct ifreq i_req;

    sprintf( i_req.ifr_name, "%s", ifname );
    if ( ioctl( *sock, SIOCGIFFLAGS, &i_req ) < 0 ) {
        fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
        return -1;
    }
    *flags = i_req.ifr_flags;
    return 0;
}

static int set_if_run_flags( int *sock, char *ifname, short stat ) {
    int r_c, is_lb, i_c;
    short flags;
    struct ifreq i_req;

    sprintf( i_req.ifr_name, "%s", ifname );
    r_c   = ic_get_run_flags( sock, ifname, &flags );
    is_lb = ic_is_loopback( sock, ifname );

    /* skip if the device is a loopback */
    if ( !r_c && !is_lb ) {
        switch ( stat ) {
            case if_down:
                i_req.ifr_flags = flags & ~IFF_UP;
                break;
            case if_up:
                i_req.ifr_flags = flags | IFF_UP;
                break;
        }
        i_c = ioctl( *sock, SIOCSIFFLAGS, &i_req );
        if ( i_c < 0 ) {
            fprintf( stderr, "iface_info() - %s\n", strerror( errno ) );
            return -1;
        }
        return 0;
    }
    fprintf( stderr, "Device is a loopback!\n" );
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

static void free_ic_info( ic_info_t *ic_info ) {
    free( ic_info->ip );
    free( ic_info->netmask );
    free( ic_info->bcast );
}
