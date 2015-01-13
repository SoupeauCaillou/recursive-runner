//
//  GameViewController.m
//  TestGame
//
//  Created by pierre-eric on 09/01/2015.
//  Copyright (c) 2015 Soupe au Caillou. All rights reserved.
//

#import "GameViewController.h"
#import <OpenGLES/ES2/glext.h>

#include "SacWrapper.h"

@interface GameViewController () {

}
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController

extern char* iosWritablePath;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    [EAGLContext setCurrentContext:self.context];
    
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDirectory, YES);
    NSString* dir = [paths objectAtIndex:0];
    iosWritablePath = strdup(dir.UTF8String);
    
    sac_init(view.bounds.size.width, view.bounds.size.height);
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setupGL
{

}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{

}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    sac_tick();
}

- (NSUInteger) supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskLandscape;
}

bool touching[3];
CGPoint position[3];
UITouch* association[3];
int iosIsTouching(int index, float* x, float* y) {
    *x = position[index].x;
    *y = position[index].y;
    return touching[index];
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch* aTouch in touches) {
        for (int i=0; i<3; i++) {
            if (association[i] == NULL) {
                association[i] = aTouch;
                touching[i] = true;
                position[i] = [aTouch locationInView:self.view];
                break;
            }
            // ignored touch
        }
    }
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch* aTouch in touches) {
        for (int i=0; i<3; i++) {
            if (association[i] == aTouch) {
                position[i] = [aTouch locationInView:self.view];
                break;
            }
        }
    }
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch* aTouch in touches) {
        for (int i=0; i<3; i++) {
            if (association[i] == aTouch) {
                association[i] = NULL;
                touching[i] = false;
                break;
            }
        }
    }
}

@end
