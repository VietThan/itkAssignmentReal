# itkAssignment-VietThan

- 33 -> 01
- 34 -> 02
- 20 -> 03
- 19 -> 04
- 41 -> 05
- 42 -> 06
- 38 -> 07
- 36 -> 08
- 35 -> 09
- 40 -> 10
- 22 -> 11
- 29 -> 12
- 27 -> 13
- 26 -> 14
- 21 -> 15
- 25 -> 16
- 11 -> 17
- 31 -> 18
- 24 -> 19
- 37 -> 20
- 17 -> 21 

## Command line

./average 21 iteration_1_deformed_image_{01..21}.nii average_iteration_1.nii
./affine 20 KKI2009-01-MPRAGE.nii KKI2009-{02..21}-MPRAGE.nii
./deformed average_affine.nii affine_KKI2009-02-MPRAGE.nii image_02.nii 100
