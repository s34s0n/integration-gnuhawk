#!/usr/bin/env python
#
# Copyright 2004,2007,2010 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

from gnuradio import gr, gr_unittest
import math

class test_filter_delay_fc (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_filter_delay_one_input (self):

        # expected result
        expected_result = (                             -1.4678005338941702e-11j,
                                                        -0.0011950774351134896j,
                                                        -0.0019336787518113852j,
                                                        -0.0034673355985432863j,
                                                        -0.0036765895783901215j,
                                                        -0.004916108213365078j,
                                                        -0.0042778430506587029j,
                                                        -0.006028641015291214j,
                                                        -0.005476709920912981j,
                                                        -0.0092810001224279404j,
                                                        -0.0095402700826525688j,
                                                        -0.016060983762145042j,
                                                        -0.016446959227323532j,
                                                        -0.02523401565849781j,
                                                        -0.024382550269365311j,
                                                        -0.035477779805660248j,
                                                        -0.033021725714206696j,
                                                        -0.048487484455108643j,
                                                        -0.04543270543217659j,
                                                        -0.069477587938308716j,
                                                        -0.066984444856643677j,
                                                        -0.10703597217798233j,
                                                        -0.10620346665382385j,
                                                        -0.1852707713842392j,
                                                        -0.19357112050056458j,
                            (7.2191945754696007e-09     -0.50004088878631592j),
                            (0.58778399229049683        -0.6155126690864563j),
                            (0.95105588436126709        -0.12377222627401352j),
                            (0.95105588436126709        +0.41524654626846313j),
                            (0.5877838134765625         +0.91611981391906738j),
                            (5.8516356205018383e-09     +1.0670661926269531j),
                            (-0.5877840518951416        +0.87856143712997437j),
                            (-0.95105588436126709       +0.35447561740875244j),
                            (-0.95105588436126709       -0.26055556535720825j),
                            (-0.5877838134765625        -0.77606213092803955j),
                            (-8.7774534307527574e-09    -0.96460390090942383j),
                            (0.58778399229049683        -0.78470128774642944j),
                            (0.95105588436126709        -0.28380891680717468j),
                            (0.95105588436126709        +0.32548999786376953j),
                            (0.5877838134765625         +0.82514488697052002j),
                            (1.4629089051254596e-08     +1.0096219778060913j),
                            (-0.5877840518951416        +0.81836479902267456j),
                            (-0.95105588436126709       +0.31451958417892456j),
                            (-0.95105588436126709       -0.3030143678188324j),
                            (-0.5877838134765625        -0.80480599403381348j),
                            (-1.7554906861505515e-08    -0.99516552686691284j),
                            (0.58778399229049683        -0.80540722608566284j),
                            (0.95105582475662231        -0.30557557940483093j),
                            (0.95105588436126709        +0.31097668409347534j),
                            (0.5877838134765625         +0.81027895212173462j),
                            (2.3406542482007353e-08     +1.0000816583633423j),
                            (-0.5877840518951416        +0.80908381938934326j),
                            (-0.95105588436126709       +0.30904293060302734j),
                            (-0.95105588436126709       -0.30904296040534973j),
                            (-0.5877838134765625        -0.80908387899398804j),
                            (-2.6332360292258272e-08    -1.0000815391540527j),
                            (0.58778399229049683        -0.80908381938934326j),
                            (0.95105582475662231        -0.30904299020767212j),
                            (0.95105588436126709        +0.30904293060302734j),
                            (0.5877838134765625         +0.80908381938934326j),
                            (3.218399768911695e-08      +1.0000815391540527j))

        tb = self.tb

        sampling_freq = 100

        ntaps = 51
        src1 = gr.sig_source_f (sampling_freq, gr.GR_SIN_WAVE,
                               sampling_freq * 0.10, 1.0)
        head = gr.head (gr.sizeof_float, int (ntaps + sampling_freq * 0.10))
        dst2 = gr.vector_sink_c ()

        # calculate taps
        taps = gr.firdes_hilbert (ntaps)
        hd = gr.filter_delay_fc (taps)

        tb.connect (src1, head)
        tb.connect (head, hd)
        tb.connect (hd,dst2)

        tb.run ()

        # get output
        result_data = dst2.data ()
        self.assertComplexTuplesAlmostEqual (expected_result, result_data, 5)

    def test_002_filter_delay_two_inputs (self):

        # giving the same signal to both the inputs should fetch the same results
        # as above

        # expected result
        expected_result = (                             -1.4678005338941702e-11j,
                                                        -0.0011950774351134896j,
                                                        -0.0019336787518113852j,
                                                        -0.0034673355985432863j,
                                                        -0.0036765895783901215j,
                                                        -0.004916108213365078j,
                                                        -0.0042778430506587029j,
                                                        -0.006028641015291214j,
                                                        -0.005476709920912981j,
                                                        -0.0092810001224279404j,
                                                        -0.0095402700826525688j,
                                                        -0.016060983762145042j,
                                                        -0.016446959227323532j,
                                                        -0.02523401565849781j,
                                                        -0.024382550269365311j,
                                                        -0.035477779805660248j,
                                                        -0.033021725714206696j,
                                                        -0.048487484455108643j,
                                                        -0.04543270543217659j,
                                                        -0.069477587938308716j,
                                                        -0.066984444856643677j,
                                                        -0.10703597217798233j,
                                                        -0.10620346665382385j,
                                                        -0.1852707713842392j,
                                                        -0.19357112050056458j,
                            (7.2191945754696007e-09     -0.50004088878631592j),
                            (0.58778399229049683        -0.6155126690864563j),
                            (0.95105588436126709        -0.12377222627401352j),
                            (0.95105588436126709        +0.41524654626846313j),
                            (0.5877838134765625         +0.91611981391906738j),
                            (5.8516356205018383e-09     +1.0670661926269531j),
                            (-0.5877840518951416        +0.87856143712997437j),
                            (-0.95105588436126709       +0.35447561740875244j),
                            (-0.95105588436126709       -0.26055556535720825j),
                            (-0.5877838134765625        -0.77606213092803955j),
                            (-8.7774534307527574e-09    -0.96460390090942383j),
                            (0.58778399229049683        -0.78470128774642944j),
                            (0.95105588436126709        -0.28380891680717468j),
                            (0.95105588436126709        +0.32548999786376953j),
                            (0.5877838134765625         +0.82514488697052002j),
                            (1.4629089051254596e-08     +1.0096219778060913j),
                            (-0.5877840518951416        +0.81836479902267456j),
                            (-0.95105588436126709       +0.31451958417892456j),
                            (-0.95105588436126709       -0.3030143678188324j),
                            (-0.5877838134765625        -0.80480599403381348j),
                            (-1.7554906861505515e-08    -0.99516552686691284j),
                            (0.58778399229049683        -0.80540722608566284j),
                            (0.95105582475662231        -0.30557557940483093j),
                            (0.95105588436126709        +0.31097668409347534j),
                            (0.5877838134765625         +0.81027895212173462j),
                            (2.3406542482007353e-08     +1.0000816583633423j),
                            (-0.5877840518951416        +0.80908381938934326j),
                            (-0.95105588436126709       +0.30904293060302734j),
                            (-0.95105588436126709       -0.30904296040534973j),
                            (-0.5877838134765625        -0.80908387899398804j),
                            (-2.6332360292258272e-08    -1.0000815391540527j),
                            (0.58778399229049683        -0.80908381938934326j),
                            (0.95105582475662231        -0.30904299020767212j),
                            (0.95105588436126709        +0.30904293060302734j),
                            (0.5877838134765625         +0.80908381938934326j),
                            (3.218399768911695e-08      +1.0000815391540527j))


        tb = self.tb

        sampling_freq = 100
        ntaps = 51
        src1 = gr.sig_source_f (sampling_freq, gr.GR_SIN_WAVE,
                               sampling_freq * 0.10, 1.0)
        head = gr.head (gr.sizeof_float, int (ntaps + sampling_freq * 0.10))
        dst2 = gr.vector_sink_c ()


        # calculate taps
        taps = gr.firdes_hilbert (ntaps)
        hd = gr.filter_delay_fc (taps, in_ports=2)

        tb.connect (src1, head)
        tb.connect (head, (hd,0))
        tb.connect (head, (hd,1))
        tb.connect (hd,dst2)
        tb.run ()

        # get output
        result_data = dst2.data ()

        self.assertComplexTuplesAlmostEqual (expected_result, result_data, 5)


    def test_003_filter_delay_two_inputs (self):

        # give two different inputs

        # expected result
        expected_result =         (                          -0.0020331963896751404j,
                                                             -0.0016448829555884004j,
                                                             -0.0032375147566199303j,
                                                             -0.0014826074475422502j,
                                                             -0.0033034090884029865j,
                                                             -0.00051144487224519253j,
                                                             -0.0043686260469257832j,
                                                             -0.0010198024101555347j,
                                                             -0.0082517862319946289j,
                                                             -0.003456643782556057j,
                                                             -0.014193611219525337j,
                                                             -0.005875137634575367j,
                                                             -0.020293503999710083j,
                                                             -0.0067503536120057106j,
                                                             -0.026798896491527557j,
                                                             -0.0073488112539052963j,
                                                             -0.037041611969470978j,
                                                             -0.010557252913713455j,
                                                             -0.055669989436864853j,
                                                             -0.018332764506340027j,
                                                             -0.089904911816120148j,
                                                             -0.033361352980136871j,
                                                             -0.16902604699134827j,
                                                             -0.074318811297416687j,
                                                             -0.58429563045501709j,
                                    (7.2191945754696007e-09  -0.35892376303672791j),
                                    (0.58778399229049683     +0.63660913705825806j),
                                    (0.95105588436126709     +0.87681591510772705j),
                                    (0.95105588436126709     +0.98705857992172241j),
                                    (0.5877838134765625      +0.55447429418563843j),
                                    (5.8516356205018383e-09  +0.026006083935499191j),
                                    (-0.5877840518951416     -0.60616838932037354j),
                                    (-0.95105588436126709    -0.9311758279800415j),
                                    (-0.95105588436126709    -0.96169203519821167j),
                                    (-0.5877838134765625     -0.57292771339416504j),
                                    (-8.7774534307527574e-09 -0.0073488391935825348j),
                                    (0.58778399229049683     +0.59720659255981445j),
                                    (0.95105588436126709     +0.94438445568084717j),
                                    (0.95105588436126709     +0.95582199096679688j),
                                    (0.5877838134765625      +0.58196049928665161j),
                                    (1.4629089051254596e-08  +0.0026587247848510742j),
                                    (-0.5877840518951416     -0.59129220247268677j),
                                    (-0.95105588436126709    -0.94841635227203369j),
                                    (-0.95105588436126709    -0.95215457677841187j),
                                    (-0.5877838134765625     -0.58535969257354736j),
                                    (-1.7554906861505515e-08 -0.00051158666610717773j),
                                    (0.58778399229049683     +0.58867418766021729j),
                                    (0.95105582475662231     +0.94965213537216187j),
                                    (0.95105588436126709     +0.95050644874572754j),
                                    (0.5877838134765625      +0.58619076013565063j),
                                    (2.3406542482007353e-08  +1.1920928955078125e-07j),
                                    (-0.5877840518951416     -0.58783555030822754j),
                                    (-0.95105588436126709    -0.95113480091094971j),
                                    (-0.95105588436126709    -0.95113474130630493j),
                                    (-0.5877838134765625     -0.58783555030822754j),
                                    (-2.6332360292258272e-08 -8.1956386566162109e-08j),
                                    (0.58778399229049683     +0.58783555030822754j),
                                    (0.95105582475662231     +0.95113474130630493j),
                                    (0.95105588436126709     +0.95113474130630493j),
                                    (0.5877838134765625      +0.58783560991287231j),
                                    (3.218399768911695e-08   +1.1920928955078125e-07j))

        tb = self.tb

        sampling_freq = 100
        ntaps = 51

        src1 = gr.sig_source_f (sampling_freq, gr.GR_SIN_WAVE,sampling_freq * 0.10, 1.0)
        src2 = gr.sig_source_f (sampling_freq, gr.GR_COS_WAVE,sampling_freq * 0.10, 1.0)

        head1 = gr.head (gr.sizeof_float, int (ntaps + sampling_freq * 0.10))
        head2 = gr.head (gr.sizeof_float, int (ntaps + sampling_freq * 0.10))

        taps = gr.firdes_hilbert (ntaps)
        hd = gr.filter_delay_fc (taps, in_ports=2)

        dst2 = gr.vector_sink_c ()

        tb.connect (src1, head1)
        tb.connect (src2, head2)

        tb.connect (head1, (hd,0))
        tb.connect (head2, (hd,1))
        tb.connect (hd, dst2)

        tb.run ()

        # get output
        result_data = dst2.data ()

        self.assertComplexTuplesAlmostEqual (expected_result, result_data, 5)


if __name__ == '__main__':
    gr_unittest.run(test_filter_delay_fc, "test_filter_delay_fc.xml")
