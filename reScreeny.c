// reScreeny by dots_tb - screenshots into game named folders
// Amazing Cleanup by Amazing Princess of Sleeping
//
// With help from folks at the CBPS: https://discord.cbps.xyz
// Idea by cuevavirus
//
// Testing team:
// 	cuevavirus
//	Nkekev
// 	Yoti

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <taihen.h>
#include <stdlib.h>
#include <sys/syslimits.h>

#include <vitasdk.h>

typedef struct SceAvImeParam2 {
	char *outpath;		// ex:photo0:/SCREENSHOT/kh/2019-11-14-195233.bmp
	uint32_t path_len;	// strlen(outpath)
	uint32_t unk_0x08[2];	// ex:0x0, 0xFFFFFFFF
	uint32_t type;		// off:0x10, ex:2
	uint32_t padding2;

	char *img_ext;		// ex:".bmp"
	uint32_t img_ext_len;
	uint32_t padding3;	// off:0x20
	char *filename;		// ex:"2019-11-14-190417"
	uint32_t fn_len;
	uint32_t padding4;
	char *titlename;	// off:0x30
	uint32_t title_len;

	void *unk_0x38;
	// more...?
} SceAvImeParam2;

typedef struct SceAvImgParam3 {	// size is 0x80?
	void *jpg_buffer; 
	uint32_t buffer_size;	// ex:0x4000
	uint32_t padding08;
	uint32_t padding0C;

	uint32_t unk_0x10;	// ex:0x17E836
	uint32_t padding14;
	uint32_t padding18;
	uint32_t type;

	char *img_ext;		// off:0x20
	uint32_t img_ext_len;
	uint32_t padding28;
	char *filename;

	uint32_t fn_len;	// off:0x30
	uint32_t padding34;
	char *titlename;
	uint32_t title_len;

	uint32_t padding50[4];	// off:0x40

	char *temp_location;	// off:0x50
	uint32_t tmp_loc_len;
	uint32_t padding58;
	uint32_t padding5C;

	void *ptr_0x60;		// ex:"2019-11-15-113750"
	void *ptr_0x64;		// path???
	uint32_t padding68;
	uint32_t unk_0x6C;	// ex:2

	void *ptr_0x70;		// path???
	void *ptr_0x74;
	char *temp_location2;	// ex:"ur0:temp/screenshot/capture.bmp"
	void *ptr_0x7C;
} SceAvImgParam3;

void sanitize(char *in, int len) {
	char il_chars[] = "<>:/\"\\|?*\n";
	
	char *ptr = in;
	
	// sanitize start
	while (ptr < in + len) {
		int i = 0, found = 0;
		while (il_chars[i] != 0) {
			if ((*ptr == il_chars[i]) || (*ptr == ' ')) {
				char *ptr2 = ptr;
				while (*ptr2 != 0) {
					*ptr2 = *(ptr2 + 1);
					ptr2++;
				}
				found = 1;
				break;
			}
			i++;
		}
		if (!found) break;
	}
	
	// sanitize rest
	while (ptr < in + len) {
		int i = 0;
		while (il_chars[i] != 0) {
			if (*ptr == il_chars[i]) {
				*ptr = ' ';
				break;
			}
			i++;
		}
		ptr++;
	}
	
	// fixup end
	while (ptr > in) {
		if (*ptr != ' ' && *ptr != 0) break;
		*ptr = 0;
		ptr--;
	}
}

SceUID cpy_img_uid;
tai_hook_ref_t cpy_img_ref;
int cpy_img_patch(int r1, SceAvImeParam2 *param2, SceAvImgParam3 *param3) {
	
	int ret;
	if(param2->type == 2) {
		SceDateTime time;
		sceRtcGetCurrentClockLocalTime(&time);
		
		char fn[0x20];
		sce_paf_private_snprintf(fn, sizeof(fn) - 1, "%04d-%02d-%02d-%02d%02d%02d-%06d",  time.year, time.month, time.day, time.hour, time.minute, time.second, time.microsecond);

/*
		if(!param2->img_ext_len) {
			param2->img_ext_len = param3->img_ext_len;
			param2->img_ext = sce_paf_private_malloc(param2->img_ext_len + 1);
			sce_paf_private_memset(param2->img_ext, 0, param2->img_ext_len  + 1);
			sce_paf_private_memcpy(param2->img_ext, param3->img_ext, param2->img_ext_len);
		}
*/

		param2->outpath = sce_paf_private_malloc(PATH_MAX);		
	
		char *titlename = (param2->title_len != 0) ? param2->titlename : "Other";
		
		sanitize(titlename, param2->title_len);

		int use_external_device = 0;
		SceIoStat stat;

		if(sceIoGetstat("sd0:", &stat) == 0){

			use_external_device = 1;

			if(sceIoGetstat("sd0:reScreeny/", &stat) != 0)
				sceIoMkdir("sd0:reScreeny/", 0666);

			param2->path_len = sce_paf_private_snprintf(
				param2->outpath,
				PATH_MAX - sizeof(fn) - param2->img_ext_len,
				"sd0:reScreeny/%s", titlename
			);
		}else{
			param2->path_len = sce_paf_private_snprintf(
				param2->outpath,
				PATH_MAX - sizeof(fn) - param2->img_ext_len,
				"photo0:/SCREENSHOT/%s", titlename
			);
		}

		if(sceIoGetstat(param2->outpath, &stat) != 0)
			sceIoMkdir(param2->outpath, (use_external_device != 0) ? 0666 : 6);

		sce_paf_private_snprintf(param2->outpath + param2->path_len, PATH_MAX - param2->path_len - 1, "/%s%s", fn, param3->img_ext);
		param2->path_len = sce_paf_private_strlen(param2->outpath);
		
		sce_paf_private_memset(param2->titlename, 0, param2->title_len);
		sce_paf_private_memcpy(param2->titlename, param3->titlename, param2->title_len);

		ret = 0;
	}else{
		ret = TAI_CONTINUE(int, cpy_img_ref, r1, param2, param3);
	}

	return ret;
}

int av_media_patch(tai_module_info_t *pInfo){

	SceUID modid = pInfo->modid;

	switch(pInfo->module_nid){
	// Devkit
	case 0x1656745F: // 3.60
	case 0x6A9DC40D: // 3.61
	case 0x1E1F5265: // 3.63
	case 0x135F2E28: // 3.65
	case 0xD55DFE9C: // 3.67
	case 0x035EAA17: // 3.68

	// Testkit
	case 0x3FE87731: // 3.60
	case 0x2FB17074: // 3.61
	case 0x061EA4BA: // 3.63
	case 0x4DDE4533: // 3.65
	case 0x787F0022: // 3.67
	case 0x62BE8716: // 3.68
		cpy_img_uid = taiHookFunctionOffset(&cpy_img_ref, modid, 0, 0x3e64, 1, cpy_img_patch);
		break;

	// Retail
	case 0x18B3FEEF: // 3.60
	case 0xE52E9179: // 3.61
	case 0x0980949E: // 3.63
	case 0x2708938B: // 3.65
	case 0xEFCA5EE7: // 3.67
	case 0xDE405E39: // 3.68
	case 0x6CF9DFFE: // 3.69
	case 0xDFB649A5: // 3.70
	case 0x3B8766B7: // 3.71
	case 0x42335CC5: // 3.72
	case 0x1E0D7085: // 3.73
		cpy_img_uid = taiHookFunctionOffset(&cpy_img_ref, modid, 0, 0x3e30, 1, cpy_img_patch);
		break;

	default:
		return -1;
	}

	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args){

	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_module_info_t);

	if(taiGetModuleInfo("SceAvMediaService", &tai_info) == 0){
		av_media_patch(&tai_info);
		return SCE_KERNEL_START_SUCCESS;
	}

	return SCE_KERNEL_START_NO_RESIDENT;
}

int module_stop(SceSize argc, const void *args){

	if(cpy_img_uid > 0)
		taiHookRelease(cpy_img_uid, cpy_img_ref);

	return SCE_KERNEL_STOP_SUCCESS;
}
