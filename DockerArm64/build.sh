rm ./sist2_arm64
cp ../sist2_arm64.gz .
gzip -d sist2_arm64.gz

version=$(./sist2_arm64 --version)

echo "Version ${version}"
docker build . -t simon987/sist2-arm64:"${version}" -t simon987/sist2-arm64:latest

docker push simon987/sist2-arm64:"${version}"
docker push simon987/sist2-arm64:latest

docker run --rm simon987/sist2-arm64 -v