mkdir -p ,gpu
for f in `ls ../examples/*.curv ../examples/tests/*.curv`; do
  b=$(basename $f .curv)
  echo $f,$b
  ../debug/curv -o ,gpu/$b.gpu $f || exit 1
  #curv -o gpu/$b.gpu $f || exit 1
done
diff -r gpu ,gpu || exit 1
