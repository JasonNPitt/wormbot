nums = [1,2,3,4,4,3,2,1,1,2,3,4]
letters = ["A", "B", "C"]
x_0 = 300
y_0 = 825
dx_well = 1135
dy_well = 1125
dx_plate = 3828
dy_plate = 5682

with open("platecoordinates.dat", "w") as f:
    for i in range(12):
        plate_x = x_0 + (i / 3) * dx_plate
        plate_y = y_0 + (i % 3) * dy_plate
        for j in range(12):
            well_x = plate_x + (j / 4) * dx_well
            well_y = plate_y + (nums[j] - 1) * dy_well

            well = letters[j / 4]

            out = str(i + 1) + well + str(nums[j]) + ","
            out += str(well_x) + "," + str(well_y) + "\n"

            f.write(out)
