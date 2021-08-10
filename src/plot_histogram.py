import numpy as np
import matplotlib.pyplot as plt
import csv
import sys

FRAME_RATE = 30
SCORE_INDEX = 2
THRESHOLD_PERCENTAGE = 0.7
MINIMUM_SHOT_LENGTH = FRAME_RATE * 2.5
INTERVAL = 10
AVERAGE_THRESHOLD = 0.1
def Read_Matching_Scores ( csv_file_path ):
	# Read csv into matching scores
	matching_scores = []
	line_counter = 0
	with open ( csv_file_path ) as csv_file:
	    feature_matching_scores = csv.reader ( csv_file, delimiter=',' )
	    for row in feature_matching_scores:
	        if ( line_counter > 0 ):
	            matching_scores.append ( float ( row [ SCORE_INDEX ] ) )
	        line_counter += 1
	return matching_scores

def Is_Change_In_Shot ( list_of_frames ):
	score_avg = sum ( list_of_frames ) / len ( list_of_frames )
	if score_avg < AVERAGE_THRESHOLD:
		return True
	else:
		return False
	# if list_of_frames.count ( 0 ) >= len ( list_of_frames ) * THRESHOLD_PERCENTAGE:
	# 	return True
	# else:
	# 	return False

scores = Read_Matching_Scores ( sys.argv [ 1 ] )

score_counter = 0
no_of_zeros = 0
current_shot_length = 0
current_shot_start = 0
current_shot_end = 0
while score_counter < len ( scores ):
	if score_counter >= INTERVAL:
		shot_change_status = Is_Change_In_Shot ( scores [ score_counter - INTERVAL : score_counter ] )

		if shot_change_status is False: # If it is the same shot
			current_shot_length += 1 # increase current shot length

		else: # If it is a different shot
			# Make sure we meet the minimum shot length
			if current_shot_length >= MINIMUM_SHOT_LENGTH:
				current_shot_end = score_counter
				# print ("Shot [" + str ( score_counter - current_shot_length ) + ", " + str ( score_counter  ) + "]")
				print ("Shot [" + str ( current_shot_start ) + ", " + str ( current_shot_end ) + "]")
				# Reset the shot and start another
				current_shot_length = 0
				current_shot_start = score_counter

	# Get ready for the next iteration
	score_counter += 1
