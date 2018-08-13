var gulp = require('gulp');
var spawn = require('child_process').spawn;

var command = 'run';
var target  = 'sdk15';

gulp.task('build',function(cb){
	var cmd = spawn('./firmware.sh', ['-c', command, '-t', target], { cwd: 'scripts/', stdio: 'inherit'});
	cmd.on(('error'), function(error) {
		console.log("Firmware call failure ", error);
		cb(error);
	});
	cmd.on(('close'), function(code) {
		console.log("Firmware script exited with code " + code);
		cb(0);
	});
});

gulp.task('watch', function() {
	gulp.watch('src/*.c', ['build']);
	gulp.watch('src/*.cpp', ['build']);
	gulp.watch('src/*/*.c', ['build']);
	gulp.watch('src/*/*.cpp', ['build']);
	gulp.watch('include/*.h', ['build']);
	gulp.watch('include/*/*.h', ['build']);
	gulp.watch('conf/cmake/CMake*', ['build']);
	gulp.watch('conf/cmake/*.conf', ['build']);
 });

gulp.task('default',['build','watch'],function(){});

