import os
import os.path as path
import subprocess
import shutil

def make_path(*components):
  return path.join(os.getcwd(), *components)


def get_engine_hash():
  os.chdir("./flutter")
  p = subprocess.Popen(["git", "rev-parse", "HEAD"], stdout=subprocess.PIPE)
  hash = p.stdout.readline().decode("utf-8")
  os.chdir("..")
  return hash[:-1]


ENGINE_HASH = get_engine_hash()
DEPLOY_PATH = make_path("deploy", ENGINE_HASH)

ICU_DATA_PATH = make_path("third_party", "icu", "flutter", "icudtl.dat")

CWD = os.getcwd()

FLUTTER_DIR = make_path("flutter")

HOST_DEBUG = make_path("out", "host_debug")
HOST_PROFILE = make_path("out", "host_profile")
HOST_RELEASE = make_path("out", "host_release")

TMP_DIR = path.join(DEPLOY_PATH, "tmp")

def execute_command(command):
  print(f"Executing command: '{command}'")
  exit_code = os.system(command)
  print(f"Command '{command}' executed with code {exit_code}")
  if exit_code != 0:
    raise SystemExit(f"Command {command} exited with code {exit_code}.")

def zip(out_archive, files=[], directories=[]):
  if len(files) == 0 and len(directories) == 0:
    raise SystemExit(
        f"No files and no directories have been provided to be zipped for {out_archive}"
    )
  if path.exists(TMP_DIR) and path.isdir(TMP_DIR):
    shutil.rmtree(TMP_DIR)
  os.mkdir(TMP_DIR)
  for file in files:
    shutil.copy(file, TMP_DIR)
  for dir in directories:
    basename = path.basename(dir)
    shutil.copytree(
        dir, path.join(TMP_DIR, basename), symlinks=True, dirs_exist_ok=True
    )
  os.chdir(TMP_DIR)
  execute_command(f'zip -9 -y -r {out_archive} .')
  os.chdir(CWD)
  shutil.rmtree(TMP_DIR)


def zip_directory_content(out_archive, directory_path):
  os.chdir(directory_path)
  execute_command(f"zip -9 -y -r {out_archive} ./")
  os.chdir(CWD)

def gn(params):
  execute_command("./flutter/tools/gn " + " ".join(params))

def build(config, targets):
  execute_command(f"ninja -C out/{config} " + " ".join(targets))

def check_cwd():
  cwd = os.getcwd()
  if not cwd.endswith("engine/src"):
    raise SystemExit("The script must run in the engine/src directory.")
  print("Script is running in engine/src")

def clean_deploy_directory():
  if path.exists(DEPLOY_PATH) and path.isdir(DEPLOY_PATH):
    shutil.rmtree(DEPLOY_PATH)
  execute_command(f"mkdir -p {DEPLOY_PATH}")

def set_use_prebuild_dart_sdk(v):
  os.environ["FLUTTER_PREBUILT_DART_SDK"] = str(v)

def build_host():
  print("Generating host_debug")
  gn([
    "--runtime-mode",
    "debug",
    "--no-lto",
    "--prebuilt-dart-sdk"
  ])
  print("Building host_debug")
  build("host_debug", [
    "flutter/build/archives:archive_gen_snapshot",
    "flutter/build/archives:artifacts",
    "flutter/build/archives:dart_sdk_archive",
    "flutter/build/archives:flutter_embedder_framework",
    "flutter/build/dart:copy_dart_sdk",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
    "flutter/tools/font-subset",
    "flutter/build/archives:flutter_patched_sdk",
  ])

  print("Generating host_profile")
  gn([
    "--runtime-mode",
    "profile",
    "--no-lto",
    "--prebuilt-dart-sdk"
  ])
  print("Building host_profile")
  build("host_profile", [
    "flutter/build/archives:archive_gen_snapshot",
    "flutter/build/archives:artifacts",
    "flutter/build/dart:copy_dart_sdk",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
    "flutter/tools/font-subset",
  ])

  print("Generating host_release")
  gn([
    "--runtime-mode",
    "release",
    "--no-lto",
    "--prebuilt-dart-sdk"
  ])
  print("Building host_release")
  build("host_release", [
    "flutter/build/archives:archive_gen_snapshot",
    "flutter/build/archives:artifacts",
    "flutter/build/dart:copy_dart_sdk",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
    "flutter/tools/font-subset",
    "flutter/build/archives:flutter_patched_sdk",
  ])

  shutil.copyfile(
    make_path(HOST_DEBUG, "zip_archives", "flutter_patched_sdk.zip"),
    make_path(DEPLOY_PATH, "flutter_patched_sdk.zip"),
  )

  shutil.copyfile(
    make_path(HOST_RELEASE, "zip_archives", "flutter_patched_sdk_product.zip"),
    make_path(DEPLOY_PATH, "flutter_patched_sdk_product.zip"),
  )

def package_macos_variant(label, arm64_out, x64_out, bucket_name):
  out_directory = make_path("out")
  label_directory = path.join(out_directory, label)
  arm64_directory = path.join(out_directory, arm64_out)
  x64_directory = path.join(out_directory, x64_out)
  bucket_directory = path.join(DEPLOY_PATH, bucket_name)

  os.makedirs(bucket_directory, exist_ok=True)

  create_macos_framework_command = " ".join([
      "python3",
      "./flutter/sky/tools/create_macos_framework.py",
      f"--dst {label_directory}",
      f"--arm64-out-dir {arm64_directory}",
      f"--x64-out-dir {x64_directory}"
  ])

  if label == "release":
    create_macos_framework_command += " --dsym --strip"

  print(f"Create macOS {label} FlutterMacOS.framework")
  execute_command(create_macos_framework_command)

  create_macos_gen_snapshot_command = " ".join([
      "python3", "./flutter/sky/tools/create_macos_gen_snapshots.py",
      f"--dst {label_directory}", f"--arm64-out-dir {arm64_directory}",
      f"--x64-out-dir {x64_directory}"
  ])

  print(f"Create macOS {label} gen_snapshot")
  execute_command(create_macos_gen_snapshot_command)

  macos_framework = make_path(bucket_directory, "FlutterMacOS.framework.zip")
  macos_framework_temp = make_path(
      bucket_directory, "FlutterMacOS.framework_tmp.zip"
  )
  zip_directory_content(
      macos_framework, path.join(label_directory, "FlutterMacOS.framework")
  )
  zip(macos_framework_temp, files=[macos_framework])
  os.remove(macos_framework)
  os.rename(macos_framework_temp, macos_framework)

  gen_snapshot_zip = path.join(bucket_directory, "gen_snapshot.zip")
  gen_snapshot_arm64 = path.join(label_directory, "gen_snapshot_arm64")
  gen_snapshot_x64 = path.join(label_directory, "gen_snapshot_x64")
  zip(gen_snapshot_zip, files=[gen_snapshot_arm64, gen_snapshot_x64])

  if label == "release":
    dsym_zip = make_path(bucket_directory, bucket_directory, "FlutterMacOS.dSYM.zip")
    dsym = make_path("out", label, "FlutterMacOS.dSYM")
    zip(dsym_zip, directories=[dsym])

def build_macos():
  print("Generating mac_debug_arm64")
  gn([
    "--mac",
    "--mac-cpu",
    "arm64",
    "--runtime-mode",
    "debug",
    "--no-lto",
    "--prebuilt-dart-sdk",
  ])
  print("Building mac_debug_arm64")
  build("mac_debug_arm64", [
    "flutter/build/archives:archive_gen_snapshot",
    "flutter/build/archives:artifacts",
    "flutter/build/archives:dart_sdk_archive",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
    "flutter/tools/font-subset"
  ])

  print("Generating mac_profile_arm64")
  gn([
    "--mac",
    "--mac-cpu",
    "arm64",
    "--runtime-mode",
    "profile",
    "--no-lto",
    "--prebuilt-dart-sdk",
  ])
  print("Building mac_profile_arm64")
  build("mac_profile_arm64", [
    "flutter/build/archives:artifacts",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
  ])

  print("Generating mac_release_arm64")
  gn([
    "--mac",
    "--mac-cpu",
    "arm64",
    "--runtime-mode",
    "release",
    "--no-lto",
    "--prebuilt-dart-sdk",
  ])
  print("Building mac_release_arm64")
  build("mac_release_arm64", [
    "flutter/build/archives:artifacts",
    "flutter/shell/platform/darwin/macos:zip_macos_flutter_framework",
  ])

  os.makedirs(make_path(DEPLOY_PATH, "darwin-x64"), exist_ok=True)
  shutil.copyfile(
    make_path(HOST_DEBUG, "zip_archives", "darwin-x64", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-x64", "artifacts.zip")
  )
  shutil.copyfile(
    make_path(HOST_DEBUG, "zip_archives", "darwin-x64", "FlutterEmbedder.framework.zip"),
    make_path(DEPLOY_PATH, "darwin-x64", "FlutterEmbedder.framework.zip")
  )
  shutil.copyfile(
    make_path(HOST_DEBUG, "zip_archives", "darwin-x64", "font-subset.zip"),
    make_path(DEPLOY_PATH, "darwin-x64", "font-subset.zip")
  )
  shutil.copyfile(
    make_path(HOST_DEBUG, "zip_archives", "dart-sdk-darwin-x64.zip"),
    make_path(DEPLOY_PATH, "dart-sdk-darwin-x64.zip")
  )

  os.makedirs(make_path(DEPLOY_PATH, "darwin-x64-profile"), exist_ok=True)
  shutil.copyfile(
    make_path(HOST_PROFILE, "zip_archives", "darwin-x64-profile", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-x64-profile", "artifacts.zip")
  )

  os.makedirs(make_path(DEPLOY_PATH, "darwin-x64-release"), exist_ok=True)
  shutil.copyfile(
    make_path(HOST_RELEASE, "zip_archives", "darwin-x64-release", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-x64-release", "artifacts.zip")
  )

  os.makedirs(make_path(DEPLOY_PATH, "darwin-arm64"), exist_ok=True)
  mac_debug_arm64_path = make_path("out", "mac_debug_arm64")
  shutil.copyfile(
    make_path(mac_debug_arm64_path, "zip_archives", "darwin-arm64", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-arm64", "artifacts.zip")
  )
  shutil.copyfile(
    make_path(mac_debug_arm64_path, "zip_archives", "darwin-arm64", "font-subset.zip"),
    make_path(DEPLOY_PATH, "darwin-arm64", "font-subset.zip")
  )
  shutil.copyfile(
    make_path(mac_debug_arm64_path, "zip_archives", "dart-sdk-darwin-arm64.zip"),
    make_path(DEPLOY_PATH, "dart-sdk-darwin-arm64.zip")
  )

  os.makedirs(make_path(DEPLOY_PATH, "darwin-arm64-profile"), exist_ok=True)
  mac_profile_arm64_path = make_path("out", "mac_profile_arm64")
  shutil.copyfile(
    make_path(mac_profile_arm64_path, "zip_archives", "darwin-arm64-profile", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-arm64-profile", "artifacts.zip")
  )

  os.makedirs(make_path(DEPLOY_PATH, "darwin-arm64-release"), exist_ok=True)
  mac_release_arm64_path = make_path("out", "mac_release_arm64")
  shutil.copyfile(
    make_path(mac_release_arm64_path, "zip_archives", "darwin-arm64-release", "artifacts.zip"),
    make_path(DEPLOY_PATH, "darwin-arm64-release", "artifacts.zip")
  )

  package_macos_variant(
    "debug",
    "mac_debug_arm64",
    "host_debug",
    "darwin-x64"
  )

  package_macos_variant(
    "profile",
    "mac_profile_arm64",
    "host_profile",
    "darwin-x64-profile"
  )

  package_macos_variant(
    "release",
    "mac_release_arm64",
    "host_release",
    "darwin-x64-release"
  )

def build_android_debug():
  variants = [
      # android_cpu, out_dir, artifact_dir, abi, targets
      ('arm', 'android_debug', 'android-arm', 'armeabi_v7a', [
        'flutter',
        'flutter/sky/dist:zip_old_location',
        'flutter/shell/platform/android:embedding_jars',
        'flutter/shell/platform/android:abi_jars'
      ]),
      ('arm64', 'android_debug_arm64', 'android-arm64', 'arm64_v8a', [
        'flutter',
        'flutter/shell/platform/android:abi_jars'
      ]),
      #('x86', 'android_debug_x86', 'android-x86', 'x86', [
      #   'flutter',
      #    'flutter/shell/platform/android:abi_jars'
      #]),
      ('x64', 'android_debug_x64', 'android-x64', 'x86_64', [
         'flutter',
          'flutter/shell/platform/android:abi_jars'
      ]),
  ]

  deploy_maven_path = make_path(DEPLOY_PATH, "download.flutter.io", "io", "flutter")
  os.makedirs(deploy_maven_path, exist_ok=True)

  for android_cpu, out_directory, artifacts_directory, abi, targets in variants:
    print(f"Generating {out_directory}")
    gn(["--android", f"--android-cpu={android_cpu}", "--no-lto"])
    build(out_directory, targets)

    out_path = make_path("out", out_directory)
    os.makedirs(make_path(DEPLOY_PATH, artifacts_directory), exist_ok=True)
    shutil.copyfile(
      make_path(out_path, "zip_archives", artifacts_directory, "artifacts.zip"),
      make_path(DEPLOY_PATH, artifacts_directory, "artifacts.zip")
    )

    shutil.copyfile(
      make_path(out_path, "zip_archives", artifacts_directory, "symbols.zip"),
      make_path(DEPLOY_PATH, artifacts_directory, "symbols.zip")
    )

    maven_out = make_path(out_path, "zip_archives", "download.flutter.io", "io", "flutter")
    maven_name = os.listdir(maven_out)[0]

    shutil.copytree(
      make_path(maven_out, maven_name),
      make_path(deploy_maven_path, maven_name)
    )
  shutil.copyfile(
    make_path("out", "android_debug", "zip_archives", "sky_engine.zip"),
    make_path(DEPLOY_PATH, "sky_engine.zip")
  )
  shutil.copyfile(
    make_path("out", "android_debug", "zip_archives", "android-javadoc.zip"),
    make_path(DEPLOY_PATH, "android-javadoc.zip")
  )

def build_android_aot():
  variants = [
    # android_cpu, out_directory, artifacts_directory, clang_directory, android_triple, abi, targets
    (
        "arm64", "android_profile_arm64", "android-arm64-profile",
        "clang_x64", "aarch64-linux-android", "arm64_v8a", "profile", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
          'flutter/shell/platform/android:analyze_snapshot'
        ]
    ),
    (
        "arm64", "android_release_arm64", "android-arm64-release",
        "clang_x64", "aarch64-linux-android", "arm64_v8a", "release", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
          'flutter/shell/platform/android:analyze_snapshot'
        ]
    ),
    (
        "arm", "android_profile", "android-arm-profile", "clang_x64",
        "arm-linux-androidabi", "armeabi_v7a", "profile", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
        ]
    ),
    (
        "arm", "android_release", "android-arm-release", "clang_x64",
        "arm-linux-androidabi", "armeabi_v7a", "release", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
        ]
    ),
    (
        "x64", "android_profile_x64", "android-x64-profile", "clang_x64",
        "x86_64-linux-android", "x86_64", "profile", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
        ]
    ),
    (
        "x64", "android_release_x64", "android-x64-release", "clang_x64",
        "x86_64-linux-android", "x86_64", "release", [
          "default",
          "flutter/lib/snapshot",
          "flutter/shell/platform/android:gen_snapshot",
          'clang_arm64/gen_snapshot',
          'flutter/shell/platform/android:abi_jars',
        ]
    ),
  ]

  deploy_maven_path = make_path(DEPLOY_PATH, "download.flutter.io", "io", "flutter")
  os.makedirs(deploy_maven_path, exist_ok=True)

  for android_cpu, out_directory, artifacts_directory, clang_directory, _, abi, runtime_mode, targets in variants:
    gn([
      "--android",
      "--runtime-mode",
      runtime_mode,
      "--android-cpu",
      android_cpu
    ])
    build(out_directory, targets)

    os.makedirs(make_path(DEPLOY_PATH, artifacts_directory))

    shutil.copyfile(
      make_path("out", out_directory, "zip_archives", artifacts_directory, "artifacts.zip"),
      make_path(DEPLOY_PATH, artifacts_directory, "artifacts.zip")
    )

    shutil.copyfile(
      make_path("out", out_directory, "zip_archives", artifacts_directory, "symbols.zip"),
      make_path(DEPLOY_PATH, artifacts_directory, "symbols.zip")
    )

    shutil.copyfile(
      make_path("out", out_directory, "zip_archives", artifacts_directory, "darwin-x64.zip"),
      make_path(DEPLOY_PATH, artifacts_directory, "darwin-x64.zip")
    )

    maven_out = make_path("out", out_directory, "zip_archives", "download.flutter.io", "io", "flutter")
    maven_name = os.listdir(maven_out)[0]
    shutil.copytree(
      make_path(maven_out, maven_name),
      make_path(deploy_maven_path, maven_name)
    )

    #if "64" in artifacts_directory:
    #  shutil.copyfile(
    #    make_path("out", out_directory, "zip_archives", "analyze-snapshot-linux-x64.zip"),
    #    make_path(DEPLOY_PATH, artifacts_directory, "analyze-snapshot-linux-x64.zip")
    #  )

def build_android():
  build_android_debug()
  build_android_aot()

def package_ios_variant(label, arm64_out, sim_x64_out, sim_arm64_out, bucket_name):
  out_directory = make_path("out")
  label_directory = make_path("out", label)
  create_ios_framework_command = " ".join([
      "./flutter/sky/tools/create_ios_framework.py",
      "--dst",
      label_directory,
      "--arm64-out-dir",
      path.join(out_directory, arm64_out),
      "--simulator-x64-out-dir",
      path.join(out_directory, sim_x64_out),
      "--simulator-arm64-out-dir",
      path.join(out_directory, sim_arm64_out),
  ])

  if label == 'release':
    create_ios_framework_command += " --dsym --strip"

  execute_command(create_ios_framework_command)

   # Package the multi-arch gen_snapshot for macOS.
  create_macos_gen_snapshot_command = " ".join([
      "./flutter/sky/tools/create_macos_gen_snapshots.py",
      '--dst',
      label_directory,
      '--arm64-out-dir',
      path.join(out_directory, arm64_out),
      "--clang-dir",
      "clang_arm64"
  ])

  execute_command(create_macos_gen_snapshot_command)

  os.makedirs(make_path(DEPLOY_PATH, bucket_name), exist_ok=True)

  zip(
      make_path(DEPLOY_PATH, bucket_name, "artifacts.zip"),
      files=[make_path(label_directory, "gen_snapshot_arm64")],
      directories=[make_path(label_directory, "Flutter.xcframework")]
  )

  if label == 'release':
    zip(
        make_path(DEPLOY_PATH, bucket_name, "Flutter.dSYM.zip"),
        directories=[path.join(label_directory, 'Flutter.dSYM')]
    )

def build_ios():
  gn([
    "--ios",
    "--runtime-mode",
    "debug",
    "--simulator",
    "--no-lto"
  ])
  build("ios_debug_sim", [])
  impellerc_path = make_path("out", "ios_debug_sim", "clang_arm64", "impellerc")

  gn([
    "--ios",
    "--runtime-mode",
    "debug",
    "--simulator",
    "--simulator-cpu=arm64",
    "--no-lto",
    "--prebuilt-impellerc",
    impellerc_path
  ])
  build("ios_debug_sim_arm64", [])

  gn([
    "--ios",
    "--runtime-mode",
    "debug",
    "--prebuilt-impellerc",
    impellerc_path
  ])
  build("ios_debug", [])

  package_ios_variant(
    "debug",
    "ios_debug",
    "ios_debug_sim",
    "ios_debug_sim_arm64",
    "ios"
  )

  gn([
    "--ios",
    "--runtime-mode",
    "profile",
    "--prebuilt-impellerc",
    impellerc_path
  ])
  build("ios_profile", [])

  package_ios_variant(
    "profile",
    "ios_profile",
    "ios_debug_sim",
    "ios_debug_sim_arm64",
    "ios-profile"
  )

  gn([
    "--ios",
    "--runtime-mode",
    "release",
    "--prebuilt-impellerc",
    impellerc_path
  ])
  build("ios_release", [])

  package_ios_variant(
    "release",
    "ios_release",
    "ios_debug_sim",
    "ios_debug_sim_arm64",
    "ios-release"
  )


def main():
  check_cwd()
  clean_deploy_directory()
  set_use_prebuild_dart_sdk(True)
  build_host()
  build_macos()
  build_android()
  build_ios()
  set_use_prebuild_dart_sdk(False)


if __name__ == "__main__":
  main()
