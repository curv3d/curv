mkdir -p ,gpu
for f in `ls ../examples/*.curv ../examples/tests/*.curv`; do
  b=$(basename $f .curv)
  echo $f
  ../debug/curv -o ,gpu/$b.gpu $f || exit 1
  #uncomment the following to regenerate the gpu dir if examples changes
  #curv -o gpu/$b.gpu $f || exit 1
done
diff -r gpu ,gpu || exit 1
