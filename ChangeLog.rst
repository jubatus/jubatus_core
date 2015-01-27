Release 0.0.7 - 2014/12/22
--------------------------

* Improvements
    * Remove error-prone default constructor (#72, #80)
    * clustering: Add test for gmm-based clustering algorithm (#66)

* Bug fixes
    * random_unlearner now take care of entries deleted by user (#60, #79)
    * Fix error message in gaussian_normalization_filter (#85, #86)

Release 0.0.6 - 2014/11/25
--------------------------

* Improvements
    * Add normalization feature (num_filter) to fv_converter (#67, #68)
    * recommender: Improve performance of inverted_index (#44, #45)
        * This work was supported by New Energy and Industrial Technology Development Organization (NEDO).
    * clustering: Support clear RPC method (#69)
    * burst: Improved parameter validation (#75, #77)
    * burst: Remove unused debug code (#71, #74)
    * Enable libstdc++ debug mode when configured using `--enable-debug` (#73)

* Bug fixes
    * recommender: Fix unlearner leaks rows when using NN-based method (fix #76)

Release 0.0.5 - 2014/10/20
--------------------------

* New machine learning service
    * Distributed Burst Detection

* Improvements
    * Removed an unnecessary typedef (#37)

Release 0.0.4 - 2014/09/29
--------------------------

* Improvements
    * Support string replacement with capture group in oniguruma-based regexp string_filter (#53)
    * Improve varidation of replacement string in re2-based regexp string_filter (#54)
    * classifier: Improve error message when invalid configuration is given (#52)

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
