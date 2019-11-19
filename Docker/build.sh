rm ./sist2
cp ../sist2 .

version=$(./sist2 --version)

echo "Version ${version}"
docker build . -t simon987/sist2:${version} -t simon987/sist2:latest
docker push simon987/sist2:${version}
docker push simon987/sist2:latest
