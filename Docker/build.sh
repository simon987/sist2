rm ./sist2 sist2_debug
cp ../sist2.gz ../sist2_debug.gz .
gzip -d sist2.gz sist2_debug.gz
strip sist2

version=$(./sist2 --version)

echo "Version ${version}"
docker build . -t simon987/sist2:${version} -t simon987/sist2:latest

docker push simon987/sist2:${version}
docker push simon987/sist2:latest

docker run --rm simon987/sist2 -v