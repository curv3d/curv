for f in `ls ../examples/*.curv`; do
  echo $f
  ../build/curv -ofrag $f > /dev/null
done
