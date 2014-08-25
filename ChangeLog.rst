Release 0.0.3 - 2014/08/25
--------------------------

* Improvements
    * Improved speed of clustering test (#48)
    * anomaly: Exposed is_updatable interface in driver (#41)

* Bug fixes
    * classifier: Fix NHERD equations (#47)
    * stat: Fix wrong error message (#42)

Release 0.0.2 - 2014/07/22
--------------------------

* Improvements
    * Support for OS X Mavericks (#20, #23)
    * Make parameter name consistent with config (#38)
    * Add ABI version number to object file (#39)
    * #34, #35, #36, #40
* Bug fixes
    * classifier, regression: Fix a misused iterator (#21)
    * Remove out-of-bound access to vectors (#25)
    * Make bit_vector safer (#33)
    * Fix test for x86 machine (#31)

Release 0.0.1 - 2014/06/23
--------------------------

* First release of jubatus_core; algorithm component of Jubatus.
* jubatus_core was separated from Jubatus 0.5.4. Changes since 0.5.4 are as follows:
    * Add unlearning feature with sticky ID handling (#4)
    * Users can now disable regexp libraries at compile time (#8)
    * Renamed methods in Nearest Neighbor module: {similar,neighbor}_row_from_data is now called {similar,neighbor}_row_from_datum (#6)
    * Message improvements (#12)
    * Support for OS X Mavericks (#11)
