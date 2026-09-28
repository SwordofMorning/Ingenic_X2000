
target=$1

cd $target

if [ -f save/etc/ssh/sshd_config ]; then
    exit
fi

if [ ! -f etc/ssh/sshd_config ]; then
    exit
fi

mkdir -p save/etc/

cd etc/

mv ssh/ ../save/etc/

ln -s ../usr/data/ssh/
