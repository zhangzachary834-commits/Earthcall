"""Independent CPU raster witness and first-seed preservation checks.
Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
"""
import random
import tempfile
import unittest
from pathlib import Path
from PIL import Image, ImageDraw
from reconstruct_flat_image import partition, build_seed, write_first_seed

class ReconstructionTests(unittest.TestCase):
    def test_random_rasters_cover_once_and_reconstruct_exactly(self):
        rng = random.Random(20261002)
        for _ in range(100):
            w, h = rng.randrange(1, 29), rng.randrange(1, 21)
            palette = [(rng.randrange(256),rng.randrange(256),rng.randrange(256),255) for _ in range(4)]
            source=Image.new('RGBA',(w,h)); source.putdata([rng.choice(palette) for _ in range(w*h)])
            rendered=Image.new('RGBA',(w,h)); draw=ImageDraw.Draw(rendered); coverage=[0]*(w*h)
            for x,y,right,bottom,color in partition(source):
                draw.rectangle((x,y,right-1,bottom-1),fill=color)
                for row in range(y,bottom):
                    for col in range(x,right): coverage[row*w+col]+=1
            self.assertEqual(source.tobytes(),rendered.tobytes())
            self.assertTrue(all(n==1 for n in coverage))
    def test_coherent_flat_region_is_one_editable_object(self):
        self.assertEqual(len(partition(Image.new('RGB',(37,19),(217,178,51)))),1)
    def test_no_silent_approximation_or_transparency_loss(self):
        image=Image.new('RGBA',(2,1),(255,0,0,255)); image.putpixel((1,0),(0,0,255,255))
        with self.assertRaises(ValueError): partition(image,1)
        image.putpixel((1,0),(0,0,255,100))
        with self.assertRaises(ValueError): partition(image)
        with self.assertRaises(ValueError): partition(image,0)
    def test_authorship_provenance_and_existing_save_survive(self):
        with tempfile.TemporaryDirectory() as temporary:
            source=Path(temporary)/'source.png'; output=Path(temporary)/'seed.json'
            Image.new('RGB',(4,2),(217,178,51)).save(source)
            with self.assertRaises(ValueError): build_seed(source,'')
            seed=build_seed(source,'Zach'); write_first_seed(output,seed)
            before=output.read_bytes()
            with self.assertRaises(FileExistsError): write_first_seed(output,{'zones':[]})
            self.assertEqual(output.read_bytes(),before)
            region=seed['zones'][0]['world']['objects'][0]
            self.assertEqual(seed['authors'],['Zach'])
            self.assertEqual(region['authoredProperties']['reconstruction.author']['v'],'Zach')
            self.assertNotIn('faceTextures',seed['zones'][0]['materials'][0])
            self.assertEqual(seed['zones'][0]['materials'][0]['baseColor'],[1,1,1])
            self.assertFalse(list(Path(temporary).glob('.reconstruction-*')))

if __name__=='__main__': unittest.main()
