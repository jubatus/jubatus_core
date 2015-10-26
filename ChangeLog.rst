Release 0.2.4 - 2015/10/26
--------------------------

* Improvements
    * nearest_neighbor: improve performance of bit_vector based methods (#188, #191, #192, #193, #194, #196)
    * nearest_neighbor: improve get_all_row_ids performance by eliminating unnecessary lock (#188, #196)
    * classifier: improve performance by using read/write lock instead of exclusive lock (#184, #197)

* Bug fixes
    * nearest_neighbor: fix bit_vector bounds checking (#198)
    * nearest_neighbor: fix bit_vector to use GCC built-in popcount only when performance improvements are expected (#188, #196)
    * nearest_neighbor: fix missing lock in pack/unpack (#188, #196)

Release 0.2.3 - 2015/09/28
--------------------------

* Improvements
    * anomaly: add ignore_kth_same_point configuration option to avoid score to become inf (#130, #134)
    * clustering: add seed configuration option (#176, #180)
    * Improve unlearner overflow error message (#178, #187)
    * Code cleanup (#179)

* Bug fixes
    * classifier: fix missing lock in local_storage_mixture::inp (#182, #183)

Release 0.2.2 - 2015/08/31
--------------------------

* Improvements
    * clustering: improved so that consistent results are returned across runs (#167, #172)
    * clusteirng: add utility tests (#173)

* Bug fixes
    * clustering: fix test conditions (#129, #174)
    * clustering: fix MIX not working as expected (#70, #175)
    * bandit: fix gamma parameter validation (#163, #169)
    * fix bit_vector parameter assertion (#170)

Release 0.2.1 - 2015/07/27
--------------------------

* Improvements
    * recommender: support unlearner in inverted_index algorithm (#51, #120)
    * classifier: expose more status values (#166)
    * Support building with Python 3.x (#30, #162)
    * Code cleanup (#161)

* Bug fixes
    * bandit: fix exp3 algorithm calculations (#157, #158)
    * bandit: fix ucb1 algorithm calculations (#159, #160)

Release 0.2.0 - 2015/06/29
--------------------------

* Improvements
    * Support non-commutative functions in combination feature (#152, #156)
    * anomaly: lof algorithm now supports overwrite method (#154)
    * classifier: driver is now thread-safe (#144)
    * Improved compatibility with GCC-5 (#155)
    * Code cleanup (#140)

* Bug fixes
    * clustering: fix k-means segmentation fault when get_nearest_center is called before clustering is performed (#150, #151)
    * Column tables now touches unlearner on MIX (#100, #113)

Release 0.1.2 - 2015/04/27
--------------------------

* Improvements
   * bandit: add assume_unrewarded option (#125, #133)
   * Improved performance of bit_vector calculations (#137)
   * Codes cleanup (#132, #141, #143)

* Bug fixes
    * bandit: reject specifying unknown arm ID in register_reward API (#138, #148)
    * bandit: fix clear API to reset arm IDs (#142, #149)
    * recommender: fix retain_projection option not working in euclid_lsh (#98, #116)
    * Fix fv_converter become unresponsive when empty datum is given (#146, #147)
    * plugin.hpp is missing from installation (#139, #145)

Release 0.1.1 - 2015/03/30
--------------------------

* Improvements
    * Move column storage directory (#118, #123)
    * classifier: install algorithm headers (#117)
    * fv_converter reports error precisely (#119)
    * Codes / comments cleanup (#96, #97, #107, #109, #114, #115, #122, #135, #136)
    * Add language declarations to wscript (#108)
    * Update to the latest waf-unittest (#128)

* Bug fixes
    * clustering: avoid clusteirng score to become NaN (#78)

Release 0.1.0 - 2015/02/23
--------------------------

* New machine learning service
    * Distributed Multi-Armed Bandit (#111)

* Improvements
    * Add combination feature (#104)
    * classifier: Add NN-based method (#83)
    * clustering: Add test for gmm-based clustering algorithm (#66)
    * nearest_neighbor: implement get_all_rows API (#58, #101)
    * Update copyright (#103, #105)

* Bug fixes
    * weight_manager now MIX correctly in recommender, nearest_neighbor, anomaly and clustering (#61, #64)
    * weight_manager is now saved in nearest_neighbor and clustering (#62, #64)
    * nearest_neighbor: weight_manager is now updated when calling set_row API (#99, #64)
    * nearest_neighbor: overwriting rows now correctly updates the specified row (#84)
    * anomaly: neighbor of updated ID is not touched when using unlearner (#92, #94)

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
