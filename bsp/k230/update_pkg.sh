#!/usr/bin/env bash

# install tools
# sudo apt-get update
# sudo apt-get upgrade -y

# sudo apt-get -qq install python3 python3-pip gcc git libncurses5-dev gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch qemu qemu-system-arm -y
# python3 -m pip install scons requests tqdm kconfiglib
# python3 -m pip install -U pyocd


if [ ! -d "packages" ];then

    DEFAULT_RTT_PACKAGE_URL=https://github.com/RT-Thread/packages.git
    ENV_URL=https://github.com/RT-Thread/env.git
    SDK_URL="https://github.com/RT-Thread/sdk.git"

    if [ $1 ] && [ $1 = --gitee ]; then
        gitee=1
        DEFAULT_RTT_PACKAGE_URL=https://gitee.com/RT-Thread-Mirror/packages.git
        ENV_URL=https://gitee.com/RT-Thread-Mirror/env.git
        SDK_URL="https://gitee.com/RT-Thread-Mirror/sdk.git"
    fi

    env_dir=$HOME/.env
    if [ -d $env_dir ]; then
        read -p '.env directory already exists. Would you like to remove and recreate .env directory? (Y/N) ' option
        if [[ "$option" =~ [Yy*] ]]; then
            rm -rf $env_dir
        fi
    fi

    if ! [ -d $env_dir ]; then
        package_url=${RTT_PACKAGE_URL:-$DEFAULT_RTT_PACKAGE_URL}
        mkdir $env_dir
        mkdir $env_dir/local_pkgs
        mkdir $env_dir/packages
        mkdir $env_dir/tools
        git clone $package_url $env_dir/packages/packages --depth=1
        echo 'source "$PKGS_DIR/packages/Kconfig"' >$env_dir/packages/Kconfig
        git clone $SDK_URL $env_dir/packages/sdk --depth=1
        git clone $ENV_URL $env_dir/tools/scripts --depth=1
        echo -e 'export PATH=`python3 -m site --user-base`/bin:$HOME/.env/tools/scripts:$PATH' >$env_dir/env.sh
    fi

    source $env_dir/env.sh
    pkgs --update

    rm $env_dir -rf
fi

#!/download cromfs tool
#linux_x64
if [ ! -f "cromfs-tool-x64" ];then
    wget https://download-redirect.rt-thread.org/download/tools/cromfs/cromfs-tool-x64 --no-check-certificate
    chmod 777 cromfs-tool-x64
fi

#linux_x86
# if [ ! -f "cromfs-tool-x86" ];then
#     wget https://download-redirect.rt-thread.org/download/tools/cromfs/cromfs-tool-x86 --no-check-certificate
#     chmod 777 cromfs-tool-x86
# fi

#windows
# wget https://download-redirect.rt-thread.org/download/tools/cromfs/cromfs-tool.exe
