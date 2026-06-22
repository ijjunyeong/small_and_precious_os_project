# small_and_precious_os_project

부트로더 및 간단한 명령어 실행이 가능한 os

## 실행 환경
QEMU emulator version 10.2.50 (setup-20260307.exe)
NASM version 3.01
Ubuntu 26.04 LTS

## 실행 방법
1. OS_File 폴더의 모든 파일 다운로드
2. Ubuntu 실행 후 디렉터리 이동
3. make 입력
4. make run 입력

#### OR (우분투가 없다면)

(qemu 설치는 공통)
1. 파일 다운로드 후 cmd 실행 후 디렉터리로 이동
2. makefile에 있는 코드 한줄씩 입력

## 명령어

help : 명령어 목록

clear : 모든 텍스트 삭제

about : os에 대한 설명

echo : 명령어 뒤의 텍스트 출

reboot : 재부팅

halt : cpu 종료
