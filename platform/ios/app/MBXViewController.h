#import <UIKit/UIKit.h>
@class MBXState;
@class MGLMapView;

@interface MBXViewController : UIViewController

@property (nonatomic) MBXState *currentState;
@property (nonatomic, readonly) IBOutlet MGLMapView *mapView;
@property (nonatomic) BOOL showsZoomLevelOrnament;
@property (nonatomic) BOOL showsTimeFrameGraph;
@property (nonatomic) BOOL debugLoggingEnabled;
@property (nonatomic) BOOL framerateMeasurementEnabled;

@end
