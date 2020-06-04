using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class ISSF {

        //target sizes per ISSF rules
        public const decimal targetSize = 170; //mm

        public const decimal outterRingPistol = 155.5m; //mm
        public const decimal ring2Pistol = 139.5m; //mm
        public const decimal ring3Pistol = 123.5m; //mm
        public const decimal ring4Pistol = 107.5m; //mm
        public const decimal ring5Pistol = 91.5m; //mm
        public const decimal ring6Pistol = 75.5m; //mm
        public const decimal ring7Pistol = 59.5m; //mm
        public const decimal ring8Pistol = 43.5m; //mm
        public const decimal ring9Pistol = 27.5m; //mm
        public const decimal ring10Pistol = 11.5m; //mm
        public const decimal innerRingPistol = 5m; //mm

        public const decimal innerTenRadiusPistol = 4.75m;

        public const float pistol1X = 5.6f; //oficial scoring dimentions
        public const float pistol1Y = 10.3f;
        public const float pistol2X = 4f;
        public const float pistol2Y = 10.5f;


        public static readonly decimal[] ringsPistol = new decimal[] { outterRingPistol, ring2Pistol, ring3Pistol, ring4Pistol, ring5Pistol, ring6Pistol, ring7Pistol, ring8Pistol, ring9Pistol, ring10Pistol, innerRingPistol };

        public const decimal outterRingRifle = 45.5m; //mm
        public const decimal ring2Rifle = 40.5m; //mm
        public const decimal ring3Rifle = 35.5m; //mm
        public const decimal ring4Rifle = 30.5m; //mm
        public const decimal ring5Rifle = 25.5m; //mm
        public const decimal ring6Rifle = 20.5m; //mm
        public const decimal ring7Rifle = 15.5m; //mm
        public const decimal ring8Rifle = 10.5m; //mm
        public const decimal ring9Rifle = 5.5m; //mm
        public const decimal ring10Rifle = 0.5m; //mm

        public const decimal innerTenRadiusRifle = 2.0m;

        public const float rifle1X = 2.50f; //oficial scoring dimentions
        public const float rifle1Y = 10.0f;
        public const float rifle2X = 2.25f;
        public const float rifle2Y = 10.1f;

        public static readonly decimal[] ringsRifle = new decimal[] { outterRingRifle, ring2Rifle, ring3Rifle, ring4Rifle, ring5Rifle, ring6Rifle, ring7Rifle, ring8Rifle, ring9Rifle, ring10Rifle };

        public const decimal pelletCaliber = 4.5m;

        public const int finalSeriesTime = 250; //seconds
        public const int singleShotTime = 50; //seconds
        public const int finalNoOfShots = 24;
        public const int match60NoOfShots = 60;
        public const int match60Time = 75;//seconds
        public const int match40NoOfShots = 40;
        public const int match40Time = 50;//seconds

        public const int pistolBlackRings = 7;
        public const int rifleBlackRings = 4;

    }
}
