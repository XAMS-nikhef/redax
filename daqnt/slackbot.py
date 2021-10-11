"""Wrapper for DaqSlackUpload to have imports in one place"""
from warnings import warn
from daq_bot import DaqSlackUpload
import typing as ty

__all__ = ['DaqntBot']


class DaqntBot(DaqSlackUpload):
    def __init__(self,
                 token: ty.Union[None, str] = None,
                 slack_channel: str = 'ch_daq_general',
                 channel_key: str = 'C01HDJCER4G'):
        """
        DAQnT specific wrapper for SlackBot
        :param token: slack API token for daq-bot
        :param slack_channel: name of the channel to post to (not
            required when channel_key is provided). Otherwise it will
            try to get the file from a utilix file.
        :param channel_key: key of the channel where to posts messages

        """
        self.to_tag = {'daq': '<!subteam^S01AVMESHAP|daq>',
                       'shifter1': '<@U01C1TGJUER|shifter1>',
                       'shifter2': '<@U01BY8LQ6QN|shifter2>',
                       'rc': '<!subteam^S01RQQMC4R0|rc>',
                       }
        super().__init__(slack_channel, token, channel_key)

    def send_message(self,
                     message: str,
                     add_tags: ty.Union[ty.Tuple[str], str] = ('daq',),
                     **kwargs,
                     ):
        """
        Wrapper to add tags to the message of the DAQ-bot
        :param message: string to post to the slack_channel
        :param add_tags: Tag any of the pre-defined users
        :param kwargs: are passed on to the super.send_message
        :return: slack response
        """
        if add_tags:
            message += '\n'

        if add_tags == 'ALL':
            for user in self.to_tag.values():
                message += f'{user} '
        elif isinstance(add_tags, (tuple, list)):
            for user_name in add_tags:
                if user_name in self.to_tag:
                    message += f'{self.to_tag[user_name]} '
                else:
                    warn(f'Unknown user {user_name}')
        elif add_tags is not None:
            warn(f'add_tags should be ALL or tuple of strings not {add_tags}')

        return super().send_message(message, **kwargs)
