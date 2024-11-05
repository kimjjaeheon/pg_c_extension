# 개요  
- PostgreSQL은 외부 json 파일을 import 할 수 있는 기능이 없다.
- OS 명령을 이용하여 carrage return을 제거하고 1 line 으로 만들어서 json 으로 인식 처리중
- PG function 에서 OS 명령 제거 필요   
  
## 방안  
- C로 extension (c라이브러리) 개발

## 기대효과
- OS 명령으로 인한 defunct/zombie 프로세스 감소
- 로딩 수행시간 감소
- 추후 기타 필요기능 개발 가능


--- 
# 사용법 NEW (2023.05.22)
1. 소스 컴파일 설치로 변경


# 사용법 NEW (2022.08.01)


## loadjson + fs_size 통합 version 
### 1. tran_extension 다운

### 2. 명령어 수행 
1. make clean
2. make
3. make install 

### 3. postgresql 접속
1. `pq` or `pqd`
2. create extension tran_extension;
3. \dx 로 tran_extension 설치 확인
4. select loadjson('{PATH}');
   select fs_size('{PATH}');
   명령어로 잘 동작하는지 확인

--- 

# 사용법 BACKUP (2022.08.01 이전)

## loadjson, loadcsv, fs_size 확장 모듈 설치 3가지 방법 (동일) 
### 1. install.sh를 이용한 설치 방법    
1. pg 버전이 12, 13버전일 경우 **loadjson_installer.tar.gz 혹은 loadcsv_installer.tar.gz 혹은 fs_size.tar.gz** 압축파일을 푼다.
	- `./install.sh -i` 명령어로 설치한다.  
	- `./install.sh -u` 명령어는 설치 삭제이다.  

  
### 2. 직접 빌드 방법
1. loadjson.tar.gz 압축파일을 푼다.
	- gunzip loadjson_installer.tar.gz
	- tar -xvf loadjson_installer.tar   
2. cd loadjson
3. make clean
4. make
	- 안될 시 makefile 열어서 PG_CONFIG 부분에 $(PG_HOME) 부분을 DB 경로를 직접 넣어준다.
5. make install
	- 안될 시 db 경로 권한 확인
6. pq
7. \dx; 
	- loadjson 모듈이 존재하면 drop extension loadjson; create extension loadjson; \dx; 
	- loadjson 모듈이 존재하지 않으면 create extension loadjson; \dx;
  
  
### 3. package 직접 푸는 설치 방법
1. **loadjson_install.tar.gz 혹은 loadcsv_installer.tar.gz 혹은 fs_size.tar.gz** 파일을 다운로드 한다.
	- 압축을 푼다.
	- 해당 Postgresql 버전에 맞는 디렉토리를 찾는다. (ver_10, ver_11, ver_12, ver_13)  
	- .control 파일, .sql 파일, .so 파일을 다운로드 한다.  
2. **".control"** 파일 
	- share/extension 디렉토리 밑으로 이동  
	(SHAREDIR/extension 디렉토리 경로와 일치)  
	ex) /usr/pgsql-10/share/extension 경로로 이동
3. **".sql"** 파일
	- share/extension 디렉토리 밑으로 이동  
	(SHAREDIR/extension 디렉토리 경로와 일치)  
	ex) /usr/pgsql-10/share/extension 경로로 이동
4. **".so"** 파일
	- lib 디렉토리 밑으로 이동    
	ex) /usr/pgsql-10/lib 경로로 이동
	  
  

## DB 접속 후 확장 모듈 사용방법
  
1. `CREATE EXTENSION 확장모듈이름;`   
	- 객체를 DB에 로드하는 명령어
	- DB 관리자 권한으로만 실행할 수 있다.
	- ex) CREATE EXTENSION loadjson;
2. `\dx`  혹은 `select * from pg_extension;`
	- 확장모듈활인 (만든 확장 모듈이 존재하는 지 확인가능)
3. `SELECT 확장모듈이름;`
	- ex) SELECT loadjson('/sw/tranm/bin/pg.sh');
	- loadjson의 경우 괄호 안에 파일경로를 넣으면 읽어온다.

***※ 확장 모듈 삭제 시 `DROP EXTENSION 확장모듈이름`***

## 확장 모듈을 만들기 위한 환경 설정  
1. `export PATH=/usr/pgsql-10/bin:$PATH`
2. `pg_config` 로 설정 확인
3. PGXS 설치 확인   
	`pg_config --pgxs` 
 
## 확장 모듈 생성 

1. **4개의 파일이 필요하다.**  
	- 이름.control 파일  
		ex) loadjson.control
	- makefile
	- 이름--버전.sql  
		ex) loadjson--0.0.1.sql
	- 이름.c 파일  
		ex) loadjson.c  
2. **이름.control 파일**  
	- 확장의 몇가지 기본 속성을 명시 해놓은 파일  
	- SHAREDIR/extension 디렉토리에 반드시 설치되어야 한다.  
3. **이름--버전.sql 파일**  
	- PostgreSQL 확장에서는 여러 SQL 객체가 필요한데, 확장 개체를 생성하기 위한 SQL 커맨드를 담는 용도의 파일
	- SHAREDIR/extension 디렉토리에 반드시 설치되어야 한다. 
4. **makefile**  
	- 빌드를 위한 파일
5. **이름.c 파일**  
	- 작동을 하기 위한 로직이 있는 파일


