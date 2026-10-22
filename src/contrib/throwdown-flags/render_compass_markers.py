"""Render the code-authored cut-paper compass SVGs and a scale-check preview."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops, ImageFont
import sys

PAPER=[(105,163),(294,169),(319,155),(491,167),(522,154),(700,167),(723,154),(933,166),
       (895,244),(903,255),(841,347),(849,360),(779,463),(787,477),(710,589),(718,600),
       (649,710),(657,722),(522,922),(488,870),(475,876),(404,752),(391,759),
       (319,630),(306,637),(231,500),(218,507),(149,371),(135,378),(91,236)]
INK=[(124,186),(297,193),(323,179),(494,191),(526,178),(702,191),(725,178),(901,189),
     (871,245),(881,256),(817,349),(826,360),(756,463),(766,477),(687,588),(696,601),
     (626,708),(634,719),(521,879),(490,833),(477,839),(426,732),(413,739),
     (340,610),(327,617),(253,481),(240,488),(171,352),(157,360),(117,235)]
FILL=[(178,241),(293,246),(309,234),(451,246),(468,234),(642,245),(659,233),(850,244),
      (813,303),(817,319),(749,429),(753,446),(683,551),(667,551),(608,662),
      (612,677),(520,822),(500,790),(488,793),(433,693),(420,694),(360,586),
      (348,589),(289,481),(276,482),(224,377),(210,382),(168,280)]
CHIPS=[[(660,266),(800,258),(781,279),(690,285)],
       [(729,302),(790,290),(771,313),(735,316)],
       [(366,603),(385,627),(380,660),(354,610)],
       [(499,759),(521,782),(520,805),(504,779)]]
SCRATCHES=[[(684,269),(774,261),(762,268),(688,281)],
           [(729,306),(769,297),(760,303),(729,312)]]

def polygon(points):return ' '.join(f'{x},{y}' for x,y in points)

def render(folder,stem,color):
    scale=2;size=1024*scale
    image=Image.new('RGBA',(size,size))
    def points(p):return [(x*scale,y*scale) for x,y in p]
    draw=ImageDraw.Draw(image)
    for shape,c in [(PAPER,(245,241,226,255)),(INK,(15,16,19,255)),(FILL,(*color,255))]:
        draw.polygon(points(shape),fill=c)
    detail=Image.new('RGBA',(size,size));d=ImageDraw.Draw(detail)
    dots=[]
    for row,y in enumerate(range(274,611,48)):
        for col,x in enumerate(range(216,499,48)):
            radius=max(5,15-col*2)
            x+=24*(row%2)
            dots.append((x,y,radius))
            d.ellipse(((x-radius)*scale,(y-radius)*scale,(x+radius)*scale,(y+radius)*scale),fill=(15,16,19,230))
    for shape in CHIPS:d.polygon(points(shape),fill=(15,16,19,255))
    for shape in SCRATCHES:d.polygon(points(shape),fill=(255,250,224,255))
    mask=Image.new('L',(size,size));ImageDraw.Draw(mask).polygon(points(FILL),fill=255)
    detail.putalpha(ImageChops.multiply(detail.getchannel('A'),mask));image.alpha_composite(detail)
    image=image.resize((1024,1024),Image.Resampling.LANCZOS)
    image.save(folder/(stem+'.png'))
    fill='#'+''.join(f'{v:02x}' for v in color)
    svg=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024">',
         '<defs><clipPath id="paint"><polygon points="'+polygon(FILL)+'"/></clipPath></defs>']
    for shape,c in [(PAPER,'#f5f1e2'),(INK,'#0f1013'),(FILL,fill)]:
        svg.append(f'<polygon fill="{c}" points="{polygon(shape)}"/>')
    svg.append('<g clip-path="url(#paint)">')
    for x,y,r in dots:svg.append(f'<circle fill="#0f1013" fill-opacity=".9" cx="{x}" cy="{y}" r="{r}"/>')
    for shape in CHIPS:svg.append('<polygon fill="#0f1013" points="'+polygon(shape)+'"/>')
    for shape in SCRATCHES:svg.append('<polygon fill="#fffae0" points="'+polygon(shape)+'"/>')
    svg.append('</g></svg>');(folder/(stem+'.svg')).write_text('\n'.join(svg)+'\n')
    return image

if __name__=='__main__':
    folder=Path(sys.argv[1]);folder.mkdir(parents=True,exist_ok=True)
    icons=[render(folder,'gate-compass',(255,221,43)),render(folder,'compass-finish-gate',(243,65,74))]
    if len(sys.argv)>2:
        preview=Image.new('RGB',(880,350),(25,30,36));d=ImageDraw.Draw(preview)
        font=ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf',21)
        small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',16)
        for n,(icon,label) in enumerate(zip(icons,['REQUIRED','FINISH'])):
            x=35+n*440
            d.text((x,18),label,font=font,fill='#faf7ef')
            preview.paste(icon.resize((224,224),Image.Resampling.LANCZOS),(x,51),icon.resize((224,224),Image.Resampling.LANCZOS))
            for width,y in [(96,59),(64,198)]:
                pos=x+270;d.line((pos-20,y+width//2,pos+width+20,y+width//2),fill='#a2a6a9',width=2)
                sample=icon.resize((width,width),Image.Resampling.LANCZOS);preview.paste(sample,(pos,y),sample)
                d.text((pos,y+width+5),f'{width} px',font=small,fill='#bfc3c8')
        d.text((35,312),'Torn paper edge • heavy ink • halftone print',font=small,fill='#c9ced2')
        preview.save(sys.argv[2])
