mkdir -p ./mnt/dev
mkdir -p ./mnt/boot

cp ./kernel.map ./mnt/boot

mkdir ./mnt/home

cp -r ../root/* ./mnt
cp -r ../base/* ./mnt