for f in `ls ../examples/*.curv`; do
  echo $f
  ../debug/curv -ogpu $f > /dev/null || exit 1
done
