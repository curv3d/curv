#!/bin/sh
if [ $# -ne 2 ]; then
  echo "Usage: $0 curv-source-file vsize"
  exit 1
fi

FILE=$1
VSIZE=$2
OUTDIR=${3:-"density"}

MARGIN=2
DIMENSIONS=$(curv -x "size(file \"$FILE\") + 2*$MARGIN*$VSIZE")
WIDTH=$(echo "$DIMENSIONS"  | jq '.[0]')
HEIGHT=$(echo "$DIMENSIONS" | jq '.[1]')
DEPTH=$(echo "$DIMENSIONS"  | jq '.[2]')
XSIZE=$(curv -x "ceil($WIDTH  / $VSIZE)")
YSIZE=$(curv -x "ceil($HEIGHT / $VSIZE)")
ZSIZE=$(curv -x "ceil($DEPTH  / $VSIZE)")
SLICES=$(curv -x "ceil($ZSIZE / $VSIZE)")

echo "shape size=$DIMENSIONS grid=[$XSIZE,$YSIZE,$ZSIZE] slices=$SLICES"

BASENAME="slice"
DIRNAME=$(dirname "$FILE")

VSIZE_METERS=$(curv -x "$VSIZE / 1000")
SLICE_AXIS=Y

cd "$DIRNAME"

for s in $(seq 0 $((SLICES - 1))); do
OUTPUT="$BASENAME"'-'"$s";

echo "slice $s";

curv                         \
-o "$OUTPUT.png"             \
-O taa=6                     \
-O aa=6                      \
-O xsize=$XSIZE              \
-O ysize=$YSIZE              \
-x "
(
  {vsize,shape,slice_idx} ->
  let
    zmin = shape.bbox.[MIN,Z] - $MARGIN*vsize;
  in make_shape {
    dist: [x,y,_,t] ->
      let
        z = zmin + vsize*slice_idx;
      in shape.dist[x,y,z,0];
    colour: p -> white;
    is_2d: true;
    bbox: [shape.bbox.[MIN] - $MARGIN*vsize, shape.bbox.[MAX] + $MARGIN*vsize];
    render: {bg: black};
  }
)
{ shape=file \"$(basename $FILE)\", vsize=$VSIZE, slice_idx=$s }";
done

n=$(ls | grep '^slice-0*\.png$' | sed -e 's/^slice-//' -e 's/\.png$//' | wc -c)
n=$(expr $n - 1)

rm -r "$OUTDIR" &> /dev/null
mkdir "$OUTDIR"
mv "$BASENAME-"*".png" "$OUTDIR/"
for i in "$OUTDIR/$BASENAME-"*".png"
do
  convert $i -colorspace gray PNG24:"$i"
done

echo "<?xml version=\"1.0\"?>

<grid version=\"1.0\" gridSizeX=\"$XSIZE\" gridSizeY=\"$ZSIZE\" gridSizeZ=\"$YSIZE\"
   voxelSize=\"$VSIZE_METERS\" subvoxelBits=\"8\" slicesOrientation=\"$SLICE_AXIS\" >

    <channels>
        <channel type=\"DENSITY\" bits=\"8\" slices=\"$OUTDIR/$BASENAME-%0${n}d.png\" />
    </channels>

    <metadata>
        <entry key=\"author\" value=\"$USER\" />
        <entry key=\"creationDate\" value=\"$(date -I)\" />
    </metadata>
</grid>" > manifest.xml;

ZIP_FILE=$(basename "$FILE" .curv).svx
rm -f "$ZIP_FILE"
zip -r "$ZIP_FILE" manifest.xml "$OUTDIR"/
rm -r manifest.xml "$OUTDIR"
