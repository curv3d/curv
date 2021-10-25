#!/bin/sh
#
# Usage: svx.sh [curv-source-file] [vsize]
# 

FILE=$1
VSIZE=$2
OUTDIR=${3:-"density"}

FDUR=0.01

DIMENSIONS=$(curv -x "file \"$FILE\" >> size")
WIDTH=$(echo "$DIMENSIONS"  | jq '.[0]')
HEIGHT=$(echo "$DIMENSIONS" | jq '.[1]')
DEPTH=$(echo "$DIMENSIONS"  | jq '.[2]')
TOTAL_DURATION=$(echo "scale=10 ; ($DEPTH / $VSIZE) * $FDUR" | bc)

BASENAME="slice"
DIRNAME=$(dirname "$FILE")
OUTPUT="$BASENAME"'-*';

XSIZE=$(echo "$WIDTH  / $VSIZE" | bc )
YSIZE=$(echo "$HEIGHT / $VSIZE" | bc )
ZSIZE=$(echo "$DEPTH  / $VSIZE" | bc )
VSIZE_METERS=$(echo "scale=10 ; $VSIZE / 1000" | bc )

cd "$DIRNAME"

curv                         \
-o "$OUTPUT.png"             \
-O taa=6                     \
-O animate="$TOTAL_DURATION" \
-O fdur=$FDUR                \
-O xsize=$XSIZE              \
-O ysize=$XSIZE              \
-x "
(
  {vsize,shape} ->
  let
    fdur = $FDUR;
    zmin = shape.bbox.[MIN,Z];
  in make_shape {
    dist: [x,y,z,t] ->
      let
        frameno = floor(t / fdur);
        zz = zmin + vsize*frameno;
      in shape.dist[x,y,zz,0];
    colour: p -> white;
    is_2d: true;
    bbox: shape.bbox;
    render: {bg: black};
  }
)
{ shape=file \"$(basename $FILE)\", vsize=$VSIZE }";

rm -r "$OUTDIR" &> /dev/null
mkdir "$OUTDIR"
mv "$BASENAME-"*".png" "$OUTDIR/"
for i in "$OUTDIR/$BASENAME-"*".png"
do
  convert $i -colorspace gray PNG24:"$i"
done

echo "<?xml version=\"1.0\"?>

<grid version=\"1.0\" gridSizeX=\"$XSIZE\" gridSizeY=\"$YSIZE\" gridSizeZ=\"$ZSIZE\"
   voxelSize=\"$VSIZE_METERS\" subvoxelBits=\"8\" slicesOrientation=\"Z\" >

    <channels>
        <channel type=\"DENSITY\" bits=\"8\" slices=\"$OUTDIR/$BASENAME-%03d.png\" />
    </channels>

    <metadata>
        <entry key=\"author\" value=\"$USER\" />
        <entry key=\"creationDate\" value=\"$(date -I)\" />
    </metadata>
</grid>" > manifest.xml;

ZIP_FILE=$(basename "$FILE" .curv).svx
rm "$ZIP_FILE"
zip -r "$ZIP_FILE" manifest.xml "$OUTDIR"/
rm -r manifest.xml "$OUTDIR"
