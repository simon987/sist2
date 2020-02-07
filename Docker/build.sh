rm ./sist2
cp ../sist2 .
strip sist2

version=$(./sist2 --version)

echo "Version ${version}"
docker build . -t simon987/sist2:${version} -t simon987/sist2:latest \
  -t docker.pkg.github.com/simon987/sist2/sist2:latest -t docker.pkg.github.com/simon987/sist2/sist2:${version}
docker push simon987/sist2:${version}
docker push simon987/sist2:latest
docker push docker.pkg.github.com/simon987/sist2/sist2:latest
docker push docker.pkg.github.com/simon987/sist2/sist2:${version}

docker run --rm -it simon987/sist2 -v