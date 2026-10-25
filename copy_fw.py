Import("env")
import os
import shutil

def copy_firmware(source, target, env):
    # 생성된 원본 .bin 파일 경로
    src_bin = str(target[0])
    # 복사 대상 위치: 프로젝트 루트 폴더 (platformio.ini가 있는 위치)
    project_dir = env.subst("$PROJECT_DIR")
    dst_bin = os.path.join(project_dir, "firmware.bin")

    # 파일 복사 수행
    shutil.copyfile(src_bin, dst_bin)
    print(f"\n==================================================")
    print(f"[AUTO-COPY] 최신 바이너리 복사 완료!")
    print(f" -> {dst_bin}")
    print(f"==================================================\n")

# .bin 바이너리 생성이 완료된 직후 copy_firmware 함수 실행
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)
