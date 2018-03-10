for f in `ls ../examples/*.curv`; do
  echo $f
  ../debug/curv -ofrag $f > /dev/null || exit 1
done
