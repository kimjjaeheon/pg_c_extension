#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"
#include <string.h>
#include "utils/builtins.h"

#include <unistd.h>
#include <mntent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <math.h>
#include <iconv.h>

PG_MODULE_MAGIC;
 
PG_FUNCTION_INFO_V1(loadjson);
PG_FUNCTION_INFO_V1(fs_size);

#define IS_EUCKR(x) ( 0xa1 <= (unsigned char)x && (unsigned char)x <= 0xff )

#define READ_BUF 4096

/* ERROR CODE */
#define ICONV_CONVERT_ERROR 1001
#define FILE_SIZE_ERROR     1002

char* fileread(char* path);
long filesize(FILE* fp);
int remove_escape_character_euckr(char* str, size_t str_len, char rep_ch);
int iconv_test(char* buf, long long len);

int64 dir_tot_size(char* path); //fs_size

iconv_t ic;

/* iconv를 통해서 문자열에 문제가 있는지 확인 */
int iconv_test(char* buf, long long len)
{
	size_t out_size = sizeof(wchar_t) * len * 4;
	char*  out_buf  = (char*)palloc(out_size);

	char* in_ptr = buf;
	char* out_ptr = out_buf;

	size_t in_size = (size_t)len;
	size_t out_buf_left = out_size;
	size_t result = 0;

	memset(out_buf, 0x00, out_size);

//elog(INFO, ">> [ iconv test start ]<<"); 

	result = iconv(ic, &in_ptr, &in_size, &out_ptr, &out_buf_left);

	pfree(out_buf);

	if (result == -1)
	{
		/* 읽을 수 없는 문자가 있는 것으로 판단 */
		return ICONV_CONVERT_ERROR;
	}

//	elog(INFO, "          iconv test success");
	return 0;	
}

char* fileread(char* path)
{
	FILE* fp;
	size_t    read_len = 0;
	char*     buf;
	char      read_line_buf[READ_BUF];
	long long line_num = 1;
	long      file_size = 0;
	int       ret = 0;

/* line 단위에 euckr 문자 제거 */
/* 통으로 읽기 -> line 단위로 읽기로 변경 후 미사용(2023.03.09) */
/*
#ifdef _EUCKR
	char* s = read_line_buf, *e = 0x00;
	size_t line  = 0;
#endif
*/

	memset(read_line_buf, 0x00, READ_BUF); 
		
	if((fp = fopen(path, "rb")) == NULL)
	{
		elog(INFO, "<loadjson log> FILE Open Fail (file : %s)", path);
		return 0x00;
	}

	file_size = filesize(fp);

	/* file size가 1Gb 이상일 때 에러처리. -> palloc의 최대 값이 1Gb */
	if (file_size >= 1000000000)
	{
		elog(INFO, "<loadjson log> file size error(file : %s, file size: %ld)", path, file_size);
		if(fp) { fclose(fp); }
		return 0x00;
	}
	else if ((500000000 < file_size) && (file_size < 1000000000)) /* file size가 500Mb 보다 크고 1Gb 작을 때 warning */
	{
		elog(INFO, "<loadjson log> please check file size(file: %s, file size : %ld", path, file_size);
		buf = (char*)palloc(file_size);
	}   
	else if ( file_size == 0 )
	{
		elog(INFO, "<loadjson log> please check file size : 0, file path : %s", path);
		if(fp) { fclose(fp); }
		return 0x00;
	}   
	else
	{
		buf = (char*)palloc(file_size +  1);
		memset(buf, 0x00, file_size); 
	}   


	/* line 단위 파일 read */
	while(fgets(read_line_buf, READ_BUF, fp))
	{
		read_len = strlen(read_line_buf);

		/* line encoding check */
		ret = iconv_test(read_line_buf, read_len);

		if (ret == ICONV_CONVERT_ERROR)
		{
			elog(INFO, "<loadjson log> ICONV ERROR (file: %s, line: %lld)", path, line_num);
		}
		else
		{
			strncat(buf, read_line_buf, read_len);
			//elog(INFO, "<loadjson log> line : [%lld]", line_num);
		}
		line_num++;
	}

	if(fp) { fclose(fp); }

	return buf;
}

/* get file size */
#if 0
long int filesize(char* path)
{
	struct stat fileinfo;

	if(stat(path, &fileinfo) < 0){
		elog(ERROR, "<loadjson log> Doesn't Get File Size");
		return 0;
	}

	return fileinfo.st_size;
}
#else
long filesize(FILE* fp)
{
	long flen = 0;
	/* file size check */
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return flen;
}
#endif

Datum
loadjson(PG_FUNCTION_ARGS)
{
	text* input_path = PG_GETARG_TEXT_P(0); //INPUT
	char *path = 0x00, *buf = 0x00;

	elog(INFO, ">> loadjson start (ver: 0.0.5)");
	elog(INFO, "   file path : %s", path);

#ifdef _EUCKR
	ic = iconv_open("EUC-KR", "EUC-KR");
#else
	ic = iconv_open("UTF-8", "UTF-8");
#endif

	if (ic == (iconv_t)(-1))
	{
		elog(INFO, ">> loadjson error end (func: iconv_open) ");
		elog(INFO, "<loadjson log> iconv_open Fail");
		PG_RETURN_NULL();
	}

	if(PG_ARGISNULL(0)){
		elog(INFO, ">> loadjson error end (func: PG_ARGISNULL) ");
		elog(ERROR, "<loadjson log> No ARG");
		iconv_close(ic);
		PG_RETURN_NULL();
	}

	path = text_to_cstring(input_path);

	if(access(path, R_OK) == -1)
	{
		elog(INFO, ">> loadjson error end (func: access) ");
		elog(ERROR, "<loadjson log> It's not file (path: %s)", path);
		iconv_close(ic);
		PG_RETURN_NULL();
	}
	else
	{
		if((buf = fileread(path)) == 0x00){
			elog(INFO, ">> loadjson error end (func: fileread) ");
			elog(ERROR, "<loadjson log> File Read ERROR (file : %s)", path);
			iconv_close(ic);
			PG_RETURN_NULL();
		}
	}

	iconv_close(ic);
	elog(INFO, ">> loadjson end ");
	PG_RETURN_TEXT_P(cstring_to_text(buf));
}

int remove_escape_character_euckr(char* str, size_t str_len, char rep_ch) {

  register size_t c = 0;
  int have_escpaed_ch = 0;
  if (!str || str_len == 0) {
    return 0;
  }

  if(rep_ch == 0x00){
	  rep_ch = 0x20;
  }

  for (; c < str_len; ++c) {
    if (iscntrl(str[c])) {
#ifdef _DEBUG
      printf("%4d: iscntrl |%02x|\n", c, (unsigned char)str[c]);
#endif
      str[c] = 0x20;
      have_escpaed_ch = 1;
    }
    else if (IS_EUCKR(str[c])) {  // 2byte character

#ifdef _DEBUG2
      printf("%4d: IS_EUCKR |%02x|\n", c, (unsigned char)str[c]);
#endif

      if (c + 1 < str_len) {
        if (!IS_EUCKR(str[c + 1])) {
#ifdef _DEBUG
          printf("%4d: Not IS_EUCKR |%02x| |%02x|\n", c + 1,
                 (unsigned char)str[c], (unsigned char)str[c + 1]);
#endif
          if (isprint(str[c + 1])) {
            str[c] = 0x20;
            have_escpaed_ch = 1;
          }
          else {
            str[c] = str[c + 1] = 0x20;
            have_escpaed_ch = 1;
          }
        }
#ifdef _DEBUG2
        else {
          printf("%4d: IS_EUCKR |%02x| => |%.*s|\n", c,
                 (unsigned char)str[c + 1], 2, str + c);
        }
#endif
	}
	else {
#ifdef _DEBUG
        printf("%4d: CUT EUCKR |%02x|\n", c, (unsigned char)str[c]);
#endif
        str[c] = 0x20;
        have_escpaed_ch = 1;
      }

      c++;
    }
    else if (!isprint(str[c])) {
#ifdef _DEBUG
      printf("%4d: Not isprint |%02x|\n", c, (unsigned char)str[c]);
#endif
      str[c] = 0x20;
      have_escpaed_ch = 1;
    }
  }

  return (have_escpaed_ch ? 1 : 0);
}

/*******  fs_size extension ******/
//get dir size
int64 dir_tot_size(char* path)
{
    struct statvfs sv;
    int64 total;

    if(statvfs(path, &sv) < 0){
        elog(ERROR, "Doesn't Get File Size");
        return 0;
    }

    total = ((long long)sv.f_blocks * sv.f_bsize );

    return total;
}

Datum
fs_size(PG_FUNCTION_ARGS)
{
    text* input_path = PG_GETARG_TEXT_P(0); //INPUT
    char* path = 0x00;
    int64 size = 0;

    if(PG_ARGISNULL(0)){
        elog(ERROR, "No ARG");
        PG_RETURN_NULL();
    }

    path = text_to_cstring(input_path);

    if(access(path, R_OK) == -1)
    {
        elog(ERROR, "It's not file/directory or please check permission");
        PG_RETURN_NULL();
    }
    else
	{
        size = dir_tot_size(path);
	}

    PG_RETURN_INT64(size);
}

