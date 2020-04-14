# A Powershell script for installing Curv on Windows.
# For best results, run it in an empty directory.
#
# Based on https://github.com/melted/get-idris/
# by Niklas Larsson <niklas@mm.st>

$current_dir = pwd
$msys = 64 # 32 to build a 32-bit Idris or 64 to build 64-bit

## Set TLS version to 1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function get-tarball {
    param([string]$url, [string]$outfile, [string]$hash)
    $exists = Test-Path $outfile
    if(!$exists) {
        Invoke-WebRequest $url -OutFile $outfile -UserAgent "Curl"
        if(Test-Path $outfile) {
            Unblock-File $outfile
        } else {
            Write-Error "failed to get file $url"
            return $false
        }
    }
    $filehash = Get-FileHash $outfile -Algorithm SHA1
    if ($filehash.Hash -ne $hash) {
        rm $outfile
        $res = $filehash.Hash
        Write-Error "Mismatching hashes for $url, expected $hash, got $res"
        return $false
    } else {
        return $true
    }
}

function extract-zip
{
    param([string]$zip, [string] $outdir)
    $has_dir=Test-Path $outdir
    if(!$has_dir) {
        mkdir $outdir
    }
    if(test-path $zip)
    {
        $shell = new-object -com shell.application
        $zipdir = $shell.NameSpace($zip)
        $target = $shell.NameSpace($outdir)
        $target.CopyHere($zipdir.Items())
    }
}

function create-dir {
    param([string]$name)
    $exists = test-path $name
    if(!$exists) {
        mkdir $name
    }
}

function create-dirs {
    create-dir downloads
    create-dir support
}

function install-ghc32 {
    $url="https://downloads.haskell.org/~ghc/8.6.3/ghc-8.6.3-i386-unknown-mingw32.tar.xz"
    $file="downloads\ghc32.tar.xz"
    $hash="ceae1078e7a2dee24eee1921e8e607e15801026c"

    if(get-tarball $url $file $hash) {
        .\support\7za x -y $file
        .\support\7za x -y ghc32.tar -omsys
        rm ghc32.tar
    }
}

function install-msys32() {
    $url="http://repo.msys2.org/distrib/i686/msys2-base-i686-20190524.tar.xz"
    $file="downloads\msys32.tar.xz"
    $hash="ff86c3e4ef8777074fd394510b95943d0c943956"
    if(get-tarball $url $file $hash) {
        .\support\7za x -y $file
        .\support\7za x -y msys32.tar
        mv msys32 msys
        rm msys32.tar
    }
}

function install-ghc64 {
    $url="https://downloads.haskell.org/~ghc/8.6.3/ghc-8.6.3-x86_64-unknown-mingw32.tar.xz"
    $file="downloads\ghc64.tar.xz"
    $hash="5a352e50ccf1deecc335300c5d8984ce863059b4"

    if(get-tarball $url $file $hash) {
        .\support\7za x -y $file
        .\support\7za x -y ghc64.tar -omsys
        rm ghc64.tar
    }
}

function install-msys64() {
    $url="http://repo.msys2.org/distrib/x86_64/msys2-base-x86_64-20190524.tar.xz"
    $file="downloads\msys64.tar.xz"
    $hash="cfe5035b1b81b43469d16bfc23be8006b9a44455"
    if(get-tarball $url $file $hash) {
        .\support\7za x -y $file
        .\support\7za x -y msys64.tar
        mv msys64 msys
        rm msys64.tar
    }
}

function install-7zip() {
    $url="https://sourceforge.net/projects/sevenzip/files/7-Zip/9.20/7za920.zip/download"
    $file="downloads\7z.zip"
    $hash="9CE9CE89EBC070FEA5D679936F21F9DDE25FAAE0"
    if (get-tarball $url $file $hash) {
        $dir = "$current_dir\support"
        $abs_file = "$current_dir\$file"
        create-dir $dir
        Extract-Zip $abs_file $dir
    }
}

function download-cabal {
    $url="https://downloads.haskell.org/~cabal/cabal-install-latest/cabal-install-3.0.0.0-x86_64-unknown-mingw32.zip"
    $file="downloads\cabal64.zip"
    $hash="44281bbed26e6f9de8366b09d193620f5ecd12bc"
    if (get-tarball $url $file $hash) {
        $dir = "$current_dir\downloads"
        $abs_file = "$current_dir\$file"
        Extract-Zip $abs_file $dir
    }
}

function run-msys-installscripts {
    .\msys\autorebase.bat
    .\msys\usr\bin\bash --login -c "exit" | Out-Null
     $current_posix=.\msys\usr\bin\cygpath.exe -u $current_dir
     $win_home = .\msys\usr\bin\cygpath.exe -u $HOME
     # $cache_file = $HOME+"\AppData\roaming\cabal\packages\hackage.haskell.org\00-index.cache"
     # if (Test-Path $cache_file) {
     #    Write-Host "Removing cabal cache"
     #    rm $cache_file
     # }

    # $ghcver = "8.6.3"
    Write-Host "Installing packages"
    $bash_paths=@"
        mkdir -p ~/bin
        echo 'export LC_ALL=C' >> ~/.bash_profile
        # echo 'export PATH=/ghc-$($ghcver)/bin:`$PATH'       >> ~/.bash_profile
        echo 'export PATH=`$HOME/bin:`$PATH'            >> ~/.bash_profile
        # echo 'export PATH=$($win_home)/AppData/Roaming/cabal/bin:`$PATH' >> ~/.bash_profile
        echo 'export CC=gcc' >> ~/.bash_profile
"@
    echo $bash_paths | Out-File -Encoding ascii temp.sh
    .\msys\usr\bin\bash -l -c "$current_posix/temp.sh" | Out-Null
    # Do the installations one at a time, pacman on msys2 tends to flake out
    # for some forking reason. A new bash helps.
    # Update twice, to catch it all if the runtime has been updated
    .\msys\usr\bin\bash -l -c "pacman -Syu --noconfirm" 
    .\msys\usr\bin\bash -l -c "pacman -Syu --noconfirm" 
    .\msys\usr\bin\bash -l -c "pacman -S --noconfirm git" | Out-Null
    .\msys\usr\bin\bash -l -c "pacman -S --noconfirm tar" | Out-Null
    .\msys\usr\bin\bash -l -c "pacman -S --noconfirm gzip" | Out-Null
    .\msys\usr\bin\bash -l -c "pacman -S --noconfirm make" | Out-Null
#    if ($msys -eq 32) {
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-i686-gcc" | Out-Null
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-i686-pkg-config" | Out-Null
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-i686-libffi" | Out-Null
#    } else {
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-x86_64-gcc" | Out-Null
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-x86_64-pkg-config" | Out-Null
#        .\msys\usr\bin\bash -l -c "pacman -S --noconfirm mingw-w64-x86_64-libffi" | Out-Null
#    }
#    # .\msys\usr\bin\bash -l -c "cp $current_posix/downloads/cabal.exe ~/bin" | Out-Null
#    $ghc_cmds=@"
#    cabal update
#    if [ -d "idris" ]; then
#        cd idris; git pull
#    else
#        git clone git://github.com/idris-lang/Idris-dev idris
#        cd idris
#    fi
#    CABALFLAGS="-fffi" make
#"@
#    echo $ghc_cmds | Out-File -Encoding ascii build-idris.sh
#
#    Write-Host "Getting and building Idris"
#    $env:MSYSTEM="MINGW$msys"
#    .\msys\usr\bin\bash -l -e -c "$current_posix/build-idris.sh" | Out-Null
}

create-dirs
echo "Getting 7-zip"
install-7zip | Out-Null
if($msys -eq 32) {
    echo "Getting msys32"
    install-msys32 | Out-Null
    # echo "Getting GHC 32-bit"
    # install-ghc32 | Out-Null
	# echo "Getting cabal.exe"
    # download-cabal | Out-Null
} else {
    echo "Getting msys64"
    install-msys64 | Out-Null
    # echo "Getting GHC 64-bit"
    # install-ghc64 | Out-Null
	# echo "Getting cabal.exe"
    # download-cabal | Out-Null
}
echo "Starting msys configuration"
run-msys-installscripts
