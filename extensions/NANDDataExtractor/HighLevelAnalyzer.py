# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting


READ_STATUS = 0x70
READ        = 0x00


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    export_file = StringSetting()

    # An optional list of types this analyzer produces, providing a way to customize the way frames are displayed in Logic 2.
    result_types = {
        'read_transaction': {
            'format': 'Read Transaction'
        }
    }

    def __init__(self):
        '''
        Initialize HLA.

        Settings can be accessed using the same name used above.
        '''
        self.tracking_read = False
        self.transaction_num = 0
        self.read_data = b''
        self.previous_frame = None

        if not self.export_file:
            raise(ValueError("Export File setting is required"))

    def decode(self, frame: AnalyzerFrame):
        '''
        Process a frame from the input analyzer, and optionally return a single `AnalyzerFrame` or a list of `AnalyzerFrame`s.

        The type and data values in `frame` will depend on the input analyzer.
        '''
        data = frame.data.get('Data', None)

        fresult = None

        # Track Read commands. Some command cycles may be proprietary, but 0x00 seems to be constant
        if frame.type == "Command" or frame.type == "End":
            # We were tracking a command, but the read is over, so reset
            # We  also have to check for Read Status as that can happen after a read command, so the data won't be flash
            # data
            if self.previous_frame and self.previous_frame.type == "Read" and self.tracking_read:
                self.tracking_read = False
                fresult = AnalyzerFrame('read_transaction', self.read_transaction_start, self.previous_frame.end_time)

                with open(f'{self.export_file}_{self.transaction_num}', 'wb') as fd:
                    fd.write(self.read_data)
                self.transaction_num += 1
                self.read_data = b''


            if data == READ_STATUS:
                self.tracking_read = False
            elif data == READ:
                print("Tracking Read")
                self.tracking_read = True
                self.read_transaction_start = frame.start_time

        if self.tracking_read and frame.type == "Read":
            self.read_data += data.to_bytes(1, 'little')

        self.previous_frame = frame
        return fresult
