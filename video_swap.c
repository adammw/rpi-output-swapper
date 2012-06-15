#include <stdio.h>
#include <stdlib.h>
#include <bcm_host.h>
#include "interface/vchiq_arm/vchiq_if.h"

#define PROG_NAME "Raspberry Pi Output Swapper"
#define VERSION "0.3.0"
#define DEPTH "16"
#define BOOL char

void trigger_sdtv_output(SDTV_MODE_T mode, SDTV_ASPECT_T aspect);
void trigger_hdmi_output(HDMI_MODE_T mode, HDMI_RES_GROUP_T group, int code);
void tvservice_init();
void call_fbset(TV_GET_STATE_RESP_T* tvstate);
TV_GET_STATE_RESP_T* get_state(BOOL print_to_screen);
void print_usage(char *executable_name);
void print_version();

int main(int argc, char **argv) {
    TV_GET_STATE_RESP_T* tvstate = NULL;
    char *executable_name = *argv;
    int type = -1, group = -1, code = -1;
    BOOL do_call_fbset = 1;
    
    if (argc < 2) {
		print_usage(executable_name);
        return -1;
    }
    
	while(argc--) {
		if (strncmp("--help", *argv, 6) == 0) {
			print_usage(executable_name);
            return 0;
		}
        if (strncmp("--version", *argv, 6) == 0) {
			print_version();
            return 0;
		}
        if (strncmp("--status", *argv, 8) == 0) {
            tvservice_init();
            tvstate = get_state(1);
            goto cleanup;
        }
        if (strncmp("--no-fbset", *argv, 10) == 0) {
            do_call_fbset=0;
        }
		if (strncmp("--sdtv", *argv, 6) == 0) {
            if (argc>=1) {
                if (strncmp("NTSC", *(argv+1), 4) == 0) {
                    type = SDTV_MODE_NTSC;
                }
                if (strncmp("NTSC-J", *(argv+1), 6) == 0) {
                    type = SDTV_MODE_NTSC_J;
                }
                if (strncmp("PAL", *(argv+1), 3) == 0) {
                    type = SDTV_MODE_PAL;
                }
                if (strncmp("PAL-M", *(argv+1), 5) == 0) {
                    type = SDTV_MODE_PAL_M;
                }
                if (type == -1 && strncmp("--", *(argv+1), 2) != 0) {
                    fprintf(stderr, "Invalid SDTV group\nAllowed values: NTSC NTSC-J PAL PAL-M\n");
                    exit(-1);
                }
                if (argc>2) {
                    if (strncmp("4:3", *(argv+2), 3) == 0) {
                        code = SDTV_ASPECT_4_3;
                    }
                    if (strncmp("14:9", *(argv+2), 4) == 0) {
                        code = SDTV_ASPECT_14_9;
                    }
                    if (strncmp("16:9", *(argv+2), 4) == 0) {
                        code = SDTV_ASPECT_16_9;
                    }
                    if (code == -1 && strncmp("--", *(argv+2), 2) != 0) {
                        fprintf(stderr, "Invalid SDTV aspect\nAllowed values: 4:3 14:9 16:9\n");
                        exit(-1);
                    }
                }
            } else { // Default to PAL 4:3
                type = SDTV_MODE_PAL;
                code = SDTV_ASPECT_4_3;
            }

            tvservice_init();
            trigger_sdtv_output(type, code);
            tvstate = get_state(1);
            if (do_call_fbset) call_fbset(tvstate);
            goto cleanup;
		}
        if (strncmp("--hdmi", *argv, 6) == 0) {
            if (argc>=2) {
                if (strncmp("CEA", *(argv+1), 3) == 0 || strncmp("HDMI", *(argv+1), 4) == 0) {
                    type = HDMI_MODE_HDMI;
                    group = HDMI_RES_GROUP_CEA;
                }
                if (strncmp("DMT", *(argv+1), 3) == 0 || strncmp("DVI", *(argv+1), 3) == 0) {
                    type = HDMI_MODE_DVI;
                    group = HDMI_RES_GROUP_DMT;
                }
                if (strncmp("CEA_3D", *(argv+1), 6) == 0 || strncmp("3D", *(argv+1), 2) == 0) {
                    type = HDMI_MODE_3D;
                    group = HDMI_RES_GROUP_CEA_3D;
                }
                if (type == -1 && strncmp("--", *(argv+1), 2) != 0) {
                    fprintf(stderr, "Invalid HDMI group\nAllowed values: CEA DMT CEA_3D\n");
                    exit(-1);
                }
                code = atoi(*(argv+2));
                if (code == 0) {
                    fprintf(stderr, "Invalid HDMI mode\n");
                    exit(-1);
                }
            }
            tvservice_init();
            trigger_hdmi_output(type, group, code);
            tvstate = get_state(1);
            if (do_call_fbset) call_fbset(tvstate);
            goto cleanup;
        }
        argv++;
    }
    fprintf(stderr,"Invalid Options\n");
    print_usage(executable_name);
cleanup:
    if (tvstate != NULL) free(tvstate);
	return -1;
}
    
void print_usage(char *executable_name) {
    printf(PROG_NAME " Version " VERSION "\n\n\
Usage: %s [OPTION]...\n\
\t--help \t\t\t: display this usage information\n\
\t--version \t\t: print version information\n\
\t--status \t\t: print current resolution / and output device\n\
\t--no-fbset \t\t: don't call fbset to reset the framebuffer\n\
\t--sdtv [mode] [aspect]\t: switch output to sdtv\n\
\t--hdmi [group] [mode] \t: switch output to hdmi with the specified mode and type,\n\t\t\t\t  otherwise defaults to the monitor's preferred mode\n\n", executable_name);
}

void print_version() {
    printf("%s\n",VERSION);
}
    
void tvservice_init() {
    VCHI_INSTANCE_T vchi_instance;
    VCHI_CONNECTION_T *vchi_connections;
    
    // initialise bcm_host
    bcm_host_init();
    
    // initialise vcos/vchi
    vcos_init();
    if (vchi_initialise(&vchi_instance) != VCHIQ_SUCCESS) {
        fprintf(stderr, "failed to open vchiq instance\n");
        exit(-2);
    }
    
    // create a vchi connection
    if ( vchi_connect( NULL, 0, vchi_instance ) != 0) {
        fprintf(stderr, "failed to connect to VCHI\n");
        exit(-3);
    }
    
    // connect to tvservice
    if ( vc_vchi_tv_init( vchi_instance, &vchi_connections, 1) != 0) {
        fprintf(stderr, "failed to connect to tvservice\n");
        exit(-4);
    }
}

void call_fbset(TV_GET_STATE_RESP_T* tvstate) {
    char command[32];
    sprintf(command, "fbset -g %4i %4i %4i %4i " DEPTH, tvstate->width, tvstate->height, tvstate->width, tvstate->height);
    system(command);
}

TV_GET_STATE_RESP_T* get_state(BOOL print_to_screen) {
    TV_GET_STATE_RESP_T* tvstate = NULL;
    if ((tvstate = malloc(sizeof(TV_GET_STATE_RESP_T))) == NULL) {
        fprintf(stderr,"Out of memory.\n");
        exit(-99);
    }
    if (vc_tv_get_state(tvstate) != 0) {
        free(tvstate);
        tvstate=NULL;
    } else if (print_to_screen) {
        printf("Currently outputting %ix%i@%iHz on ",tvstate->width,tvstate->height,tvstate->frame_rate);
        if ((tvstate->state & VC_HDMI_DVI) == VC_HDMI_DVI ||
            (tvstate->state & VC_HDMI_DVI) == VC_HDMI_HDMI) {
            printf("HDMI");
        } else if ((tvstate->state & VC_SDTV_NTSC) == VC_SDTV_NTSC) {
            printf("SDTV (NTSC)");
        } else if ((tvstate->state & VC_SDTV_PAL) == VC_SDTV_PAL) {
            printf("SDTV (PAL)"); 
        } else {
            printf("nothing");
        }
        printf("\n");
    }
    return tvstate;
}

void trigger_sdtv_output(SDTV_MODE_T mode, SDTV_ASPECT_T aspect) {
	int result;
	SDTV_OPTIONS_T options = {.aspect = aspect};
	if ((result = vc_tv_sdtv_power_on( mode, &options)) != 0) {
		fprintf(stderr, "failed to turn on sdtv output: error code %i\n", result);
        exit(-5);
	} else {
		printf("sdtv output successful\n");
	}
}

void trigger_hdmi_output(HDMI_MODE_T mode, HDMI_RES_GROUP_T group, int code) {
	int result;
    if (mode == -1) {
	    result = vc_tv_hdmi_power_on_preferred();
    } else {
        result = vc_tv_hdmi_power_on_explicit(mode, group, code);
    }
    if (result != 0) {
        fprintf(stderr, "failed to turn on hdmi output: error code %i\n", result);
        exit(-5);
    } else {
        printf("hdmi output successful\n");
    }
}
